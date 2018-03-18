#pragma region Copyright (c) 2014-2017 OpenRCT2 Developers
/*****************************************************************************
 * OpenRCT2, an open source clone of Roller Coaster Tycoon 2.
 *
 * OpenRCT2 is the work of many authors, a full list can be found in contributors.md
 * For more information, visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * A full copy of the GNU General Public License can be found in licence.txt
 *****************************************************************************/
#pragma endregion

#include <algorithm>
#include <unordered_map>
#include "../core/IStream.hpp"
#include "../core/Memory.hpp"
#include "../core/String.hpp"
#include "../OpenRCT2.h"
#include "ObjectRepository.h"
#include "RideObject.h"

#include "../ride/RideGroupManager.h"
#include "../drawing/Drawing.h"
#include "../localisation/Language.h"
#include "../rct2/RCT2.h"
#include "../ride/Ride.h"
#include "../ride/Track.h"
#include "../OpenRCT2.h"
#include "ObjectJsonHelpers.h"
#include "ObjectRepository.h"
#include "RideObject.h"

using namespace OpenRCT2;

RideObject::~RideObject()
{
    for (auto &peepLoadingPosition : _peepLoadingPositions)
    {
        Memory::Free(peepLoadingPosition);
    }
}

void RideObject::ReadLegacy(IReadObjectContext * context, IStream * stream)
{
    stream->Seek(8, STREAM_SEEK_CURRENT);
    _legacyType.flags = stream->ReadValue<uint32>();
    for (auto &rideType : _legacyType.ride_type)
    {
        rideType = stream->ReadValue<uint8>();
    }
    _legacyType.min_cars_in_train = stream->ReadValue<uint8>();
    _legacyType.max_cars_in_train = stream->ReadValue<uint8>();
    _legacyType.cars_per_flat_ride = stream->ReadValue<uint8>();
    _legacyType.zero_cars = stream->ReadValue<uint8>();
    _legacyType.tab_vehicle = stream->ReadValue<uint8>();
    _legacyType.default_vehicle = stream->ReadValue<uint8>();
    _legacyType.front_vehicle = stream->ReadValue<uint8>();
    _legacyType.second_vehicle = stream->ReadValue<uint8>();
    _legacyType.rear_vehicle = stream->ReadValue<uint8>();
    _legacyType.third_vehicle = stream->ReadValue<uint8>();
    _legacyType.pad_019 = stream->ReadValue<uint8>();
    for (auto &vehicleEntry : _legacyType.vehicles)
    {
        ReadLegacyVehicle(context, stream, &vehicleEntry);
    }
    stream->Seek(4, STREAM_SEEK_CURRENT);
    _legacyType.excitement_multiplier = stream->ReadValue<sint8>();
    _legacyType.intensity_multiplier = stream->ReadValue<sint8>();
    _legacyType.nausea_multiplier = stream->ReadValue<sint8>();
    _legacyType.max_height = stream->ReadValue<uint8>();
    _legacyType.enabledTrackPieces = stream->ReadValue<uint64>();
    _legacyType.category[0] = stream->ReadValue<uint8>();
    _legacyType.category[1] = stream->ReadValue<uint8>();
    _legacyType.shop_item = stream->ReadValue<uint8>();
    _legacyType.shop_item_secondary = stream->ReadValue<uint8>();

    GetStringTable()->Read(context, stream, OBJ_STRING_ID_NAME);
    GetStringTable()->Read(context, stream, OBJ_STRING_ID_DESCRIPTION);
    GetStringTable()->Read(context, stream, OBJ_STRING_ID_CAPACITY);

    // Read preset colours, by default there are 32
    _presetColours.count = stream->ReadValue<uint8>();

    sint32 coloursCount = _presetColours.count;
    // To indicate a ride has different colours each train the count
    // is set to 255. There are only actually 32 colours though.
    if (coloursCount == 255)
    {
        coloursCount = 32;
    }

    for (uint8 i = 0; i < coloursCount; i++)
    {
        _presetColours.list[i] = stream->ReadValue<vehicle_colour>();
    }

    // Read peep loading positions
    for (sint32 i = 0; i < RCT2_MAX_VEHICLES_PER_RIDE_ENTRY; i++)
    {
        uint16 numPeepLoadingPositions = stream->ReadValue<uint8>();
        if (numPeepLoadingPositions == 255)
        {
            numPeepLoadingPositions = stream->ReadValue<uint16>();
        }
        _peepLoadingPositions[i] = stream->ReadArray<sint8>(numPeepLoadingPositions);
        _peepLoadingPositionsCount[i] = numPeepLoadingPositions;
    }

    GetImageTable()->Read(context, stream);

    // Validate properties
    if (_legacyType.excitement_multiplier > 75)
    {
        context->LogError(OBJECT_ERROR_INVALID_PROPERTY, "Excitement multiplier too high.");
    }
    if (_legacyType.intensity_multiplier > 75)
    {
        context->LogError(OBJECT_ERROR_INVALID_PROPERTY, "Intensity multiplier too high.");
    }
    if (_legacyType.nausea_multiplier > 75)
    {
        context->LogError(OBJECT_ERROR_INVALID_PROPERTY, "Nausea multiplier too high.");
    }

    PerformFixes();
}

void RideObject::Load()
{
    GetStringTable()->Sort();
    _legacyType.naming.name = language_allocate_object_string(GetName());
    _legacyType.naming.description = language_allocate_object_string(GetDescription());
    _legacyType.capacity = language_allocate_object_string(GetCapacity());
    _legacyType.images_offset = gfx_object_allocate_images(GetImageTable()->GetImages(), GetImageTable()->GetCount());
    _legacyType.vehicle_preset_list = &_presetColours;

    sint32 cur_vehicle_images_offset = _legacyType.images_offset + MAX_RIDE_TYPES_PER_RIDE_ENTRY;
    for (sint32 i = 0; i < RCT2_MAX_VEHICLES_PER_RIDE_ENTRY; i++)
    {
        rct_ride_entry_vehicle * vehicleEntry = &_legacyType.vehicles[i];
        if (vehicleEntry->sprite_flags & VEHICLE_SPRITE_FLAG_FLAT)
        {
            // RCT2 calculates num_vertical_frames and num_horizontal_frames and overwrites these properties on the vehicle entry.
            // Immediately afterwards, the two were multiplied in order to calculate base_num_frames and were never used again.
            // This has been changed to use the calculation results directly -
            // num_vertical_frames and num_horizontal_frames are no longer set on the vehicle entry.
            // 0x6DE946
            vehicleEntry->base_num_frames = CalculateNumVerticalFrames(vehicleEntry) * CalculateNumHorizontalFrames(vehicleEntry);
            vehicleEntry->base_image_id = cur_vehicle_images_offset;
            sint32 image_index = vehicleEntry->base_image_id;

            if (vehicleEntry->car_visual != VEHICLE_VISUAL_RIVER_RAPIDS)
            {
                sint32 b = vehicleEntry->base_num_frames * 32;

                if (vehicleEntry->flags & VEHICLE_ENTRY_FLAG_11) b /= 2;
                if (vehicleEntry->sprite_flags & VEHICLE_SPRITE_FLAG_15) b /= 8;

                image_index += b;

                if (vehicleEntry->sprite_flags & VEHICLE_SPRITE_FLAG_GENTLE_SLOPES)
                {
                    vehicleEntry->gentle_slope_image_id = image_index;
                    b = vehicleEntry->base_num_frames * 72;
                    if (vehicleEntry->flags & VEHICLE_ENTRY_FLAG_SPINNING_ADDITIONAL_FRAMES)
                    {
                        b = vehicleEntry->base_num_frames * 16;
                    }
                    image_index += b;
                }

                if (vehicleEntry->sprite_flags & VEHICLE_SPRITE_FLAG_STEEP_SLOPES)
                {
                    vehicleEntry->steep_slope_image_id = image_index;
                    b = vehicleEntry->base_num_frames * 80;
                    image_index += b;
                }

                if (vehicleEntry->sprite_flags & VEHICLE_SPRITE_FLAG_VERTICAL_SLOPES)
                {
                    vehicleEntry->vertical_slope_image_id = image_index;
                    b = vehicleEntry->base_num_frames * 116;
                    image_index += b;
                }

                if (vehicleEntry->sprite_flags & VEHICLE_SPRITE_FLAG_DIAGONAL_SLOPES)
                {
                    vehicleEntry->diagonal_slope_image_id = image_index;
                    b = vehicleEntry->base_num_frames * 24;
                    image_index += b;
                }

                if (vehicleEntry->sprite_flags & VEHICLE_SPRITE_FLAG_FLAT_BANKED)
                {
                    vehicleEntry->banked_image_id = image_index;
                    b = vehicleEntry->base_num_frames * 80;
                    image_index += b;
                }

                if (vehicleEntry->sprite_flags & VEHICLE_SPRITE_FLAG_INLINE_TWISTS)
                {
                    vehicleEntry->inline_twist_image_id = image_index;
                    b = vehicleEntry->base_num_frames * 40;
                    image_index += b;
                }

                if (vehicleEntry->sprite_flags & VEHICLE_SPRITE_FLAG_FLAT_TO_GENTLE_SLOPE_BANKED_TRANSITIONS)
                {
                    vehicleEntry->flat_to_gentle_bank_image_id = image_index;
                    b = vehicleEntry->base_num_frames * 128;
                    image_index += b;
                }

                if (vehicleEntry->sprite_flags & VEHICLE_SPRITE_FLAG_DIAGONAL_GENTLE_SLOPE_BANKED_TRANSITIONS)
                {
                    vehicleEntry->diagonal_to_gentle_slope_bank_image_id = image_index;
                    b = vehicleEntry->base_num_frames * 16;
                    image_index += b;
                }

                if (vehicleEntry->sprite_flags & VEHICLE_SPRITE_FLAG_GENTLE_SLOPE_BANKED_TRANSITIONS)
                {
                    vehicleEntry->gentle_slope_to_bank_image_id = image_index;
                    b = vehicleEntry->base_num_frames * 16;
                    image_index += b;
                }

                if (vehicleEntry->sprite_flags & VEHICLE_SPRITE_FLAG_GENTLE_SLOPE_BANKED_TURNS)
                {
                    vehicleEntry->gentle_slope_bank_turn_image_id = image_index;
                    b = vehicleEntry->base_num_frames * 128;
                    image_index += b;
                }

                if (vehicleEntry->sprite_flags & VEHICLE_SPRITE_FLAG_FLAT_TO_GENTLE_SLOPE_WHILE_BANKED_TRANSITIONS)
                {
                    vehicleEntry->flat_bank_to_gentle_slope_image_id = image_index;
                    b = vehicleEntry->base_num_frames * 16;
                    image_index += b;
                }

                if (vehicleEntry->sprite_flags & VEHICLE_SPRITE_FLAG_CORKSCREWS)
                {
                    vehicleEntry->corkscrew_image_id = image_index;
                    b = vehicleEntry->base_num_frames * 80;
                    image_index += b;
                }

                if (vehicleEntry->sprite_flags & VEHICLE_SPRITE_FLAG_RESTRAINT_ANIMATION)
                {
                    vehicleEntry->restraint_image_id = image_index;
                    b = vehicleEntry->base_num_frames * 12;
                    image_index += b;
                }

                if (vehicleEntry->sprite_flags & VEHICLE_SPRITE_FLAG_CURVED_LIFT_HILL)
                {
                    // Same offset as corkscrew
                    vehicleEntry->curved_lift_hill_image_id = image_index;
                    b = vehicleEntry->base_num_frames * 32;
                    image_index += b;
                }
            }
            else
            {
                image_index += vehicleEntry->base_num_frames * 36;
            }

            // No vehicle images
            vehicleEntry->no_vehicle_images = image_index - cur_vehicle_images_offset;

            // Move the offset over this vehicles images. Including peeps
            cur_vehicle_images_offset = image_index + vehicleEntry->no_seating_rows * vehicleEntry->no_vehicle_images;
            // 0x6DEB0D

            if (!(vehicleEntry->flags & VEHICLE_ENTRY_FLAG_10))
            {
                sint32 num_images = cur_vehicle_images_offset - vehicleEntry->base_image_id;
                if (vehicleEntry->flags & VEHICLE_ENTRY_FLAG_13)
                {
                    num_images *= 2;
                }

                if (!gOpenRCT2NoGraphics) {
                    set_vehicle_type_image_max_sizes(vehicleEntry, num_images);
                }
            }
            vehicleEntry->peep_loading_positions = _peepLoadingPositions[i];
            vehicleEntry->peep_loading_positions_count = _peepLoadingPositionsCount[i];
        }
    }
}

void RideObject::Unload()
{
    language_free_object_string(_legacyType.naming.name);
    language_free_object_string(_legacyType.naming.description);
    language_free_object_string(_legacyType.capacity);
    gfx_object_free_images(_legacyType.images_offset, GetImageTable()->GetCount());

    _legacyType.naming.name = 0;
    _legacyType.naming.description = 0;
    _legacyType.capacity = 0;
    _legacyType.images_offset = 0;
}

void RideObject::DrawPreview(rct_drawpixelinfo * dpi, sint32 width, sint32 height) const
{
    uint32 imageId = _legacyType.images_offset;

    for (auto rideType : _legacyType.ride_type)
    {
        if (rideType != RIDE_TYPE_NULL)
            break;
        else
            imageId++;
    }

    gfx_draw_sprite(dpi, imageId, 0, 0, 0);
}

std::string RideObject::GetDescription() const
{
    return GetString(OBJ_STRING_ID_DESCRIPTION);
}

std::string RideObject::GetCapacity() const
{
    return GetString(OBJ_STRING_ID_CAPACITY);
}

void RideObject::SetRepositoryItem(ObjectRepositoryItem * item) const
{
    for (sint32 i = 0; i < RCT2_MAX_RIDE_TYPES_PER_RIDE_ENTRY; i++)
    {
        item->RideType[i] = _legacyType.ride_type[i];
    }
    for (sint32 i = 0; i < RCT2_MAX_CATEGORIES_PER_RIDE; i++)
    {
        item->RideCategory[i] = _legacyType.category[i];
    }

    item->RideFlags = 0;

    // Find the first non-null ride type, to be used when checking the ride group
    uint8 rideTypeIdx = ride_entry_get_first_non_null_ride_type(&_legacyType);

    // Determines the ride group. Will fall back to 0 if there is none found.
    uint8 rideGroupIndex = 0;

    const RideGroup * rideGroup = RideGroupManager::GetRideGroup(rideTypeIdx, &_legacyType);

    // If the ride group is nullptr, the track type does not have ride groups.
    if (rideGroup != nullptr)
    {
        for (uint8 i = rideGroupIndex + 1; i < MAX_RIDE_GROUPS_PER_RIDE_TYPE; i++)
        {
            const RideGroup * irg = RideGroupManager::RideGroupFind(rideTypeIdx, i);

            if (irg != nullptr)
            {
                if (RideGroupManager::RideGroupsAreEqual(irg, rideGroup))
                {
                    rideGroupIndex = i;
                    break;
                }
            }
        }
    }

    item->RideGroupIndex = rideGroupIndex;
}

void RideObject::ReadLegacyVehicle(IReadObjectContext * context, IStream * stream, rct_ride_entry_vehicle * vehicle)
{
    vehicle->rotation_frame_mask = stream->ReadValue<uint16>();
    vehicle->num_vertical_frames = stream->ReadValue<uint8>();
    vehicle->num_horizontal_frames = stream->ReadValue<uint8>();
    vehicle->spacing = stream->ReadValue<uint32>();
    vehicle->car_mass = stream->ReadValue<uint16>();
    vehicle->tab_height = stream->ReadValue<sint8>();
    vehicle->num_seats = stream->ReadValue<uint8>();
    vehicle->sprite_flags = stream->ReadValue<uint16>();
    vehicle->sprite_width = stream->ReadValue<uint8>();
    vehicle->sprite_height_negative = stream->ReadValue<uint8>();
    vehicle->sprite_height_positive = stream->ReadValue<uint8>();
    vehicle->animation = stream->ReadValue<uint8>();
    vehicle->flags = stream->ReadValue<uint32>();
    vehicle->base_num_frames = stream->ReadValue<uint16>();
    stream->Seek(4, STREAM_SEEK_CURRENT);
    vehicle->restraint_image_id = stream->ReadValue<uint32>();
    vehicle->gentle_slope_image_id = stream->ReadValue<uint32>();
    vehicle->steep_slope_image_id = stream->ReadValue<uint32>();
    vehicle->vertical_slope_image_id = stream->ReadValue<uint32>();
    vehicle->diagonal_slope_image_id = stream->ReadValue<uint32>();
    vehicle->banked_image_id = stream->ReadValue<uint32>();
    vehicle->inline_twist_image_id = stream->ReadValue<uint32>();
    vehicle->flat_to_gentle_bank_image_id = stream->ReadValue<uint32>();
    vehicle->diagonal_to_gentle_slope_bank_image_id = stream->ReadValue<uint32>();
    vehicle->gentle_slope_to_bank_image_id = stream->ReadValue<uint32>();
    vehicle->gentle_slope_bank_turn_image_id = stream->ReadValue<uint32>();
    vehicle->flat_bank_to_gentle_slope_image_id = stream->ReadValue<uint32>();
    vehicle->corkscrew_image_id = stream->ReadValue<uint32>();
    vehicle->no_vehicle_images = stream->ReadValue<uint32>();
    vehicle->no_seating_rows = stream->ReadValue<uint8>();
    vehicle->spinning_inertia = stream->ReadValue<uint8>();
    vehicle->spinning_friction = stream->ReadValue<uint8>();
    vehicle->friction_sound_id = stream->ReadValue<uint8>();
    vehicle->log_flume_reverser_vehicle_type = stream->ReadValue<uint8>();
    vehicle->sound_range = stream->ReadValue<uint8>();
    vehicle->double_sound_frequency = stream->ReadValue<uint8>();
    vehicle->powered_acceleration = stream->ReadValue<uint8>();
    vehicle->powered_max_speed = stream->ReadValue<uint8>();
    vehicle->car_visual = stream->ReadValue<uint8>();
    vehicle->effect_visual = stream->ReadValue<uint8>();
    vehicle->draw_order = stream->ReadValue<uint8>();
    vehicle->num_vertical_frames_override = stream->ReadValue<uint8>();
    stream->Seek(4, STREAM_SEEK_CURRENT);
}

void RideObject::PerformFixes()
{
    std::string identifier = GetIdentifier();
    
    // Add boosters if the track type is eligible
    for (auto rideType : _legacyType.ride_type)
    {
        if (ride_type_supports_boosters(rideType))
        {
            _legacyType.enabledTrackPieces |= (1ULL << TRACK_BOOSTER);
        }
    }
    
    // The rocket cars could take 3 cars per train in RCT1. Restore this.
    if (String::Equals(identifier, "RCKC    "))
    {
        _legacyType.max_cars_in_train = 3 + _legacyType.zero_cars;
    }
    // The Wooden Roller Coaster could take 7 cars per train in RCT1.
    else if (String::Equals(identifier, "PTCT1   "))
    {
        _legacyType.max_cars_in_train = 7 + _legacyType.zero_cars;
    }
    // The Looping Roller Coaster could take 8 cars per train in RCT1.
    else if (String::Equals(identifier, "SCHT1   "))
    {
        _legacyType.max_cars_in_train = 8 + _legacyType.zero_cars;
    }
    // The Steel Twister could take 8 cars per train in RCT1. (The other two vehicles are already correct.)
    else if (String::Equals(identifier, "BMSD    ") || String::Equals(identifier, "BMSU    "))
    {
        _legacyType.max_cars_in_train = 8 + _legacyType.zero_cars;
    }
    // Wacky Worlds' Crocodile Ride (a log flume vehicle) is incorrectly locked to 5 cars.
    else if (String::Equals(identifier, "CROCFLUM"))
    {
        _legacyType.cars_per_flat_ride = 0xFF;
    }
    // All vanilla/WW/OCC Junior RC vehicles incorrectly have this flag set
    else if (String::Equals(identifier, "ZLDB    ") ||
             String::Equals(identifier, "ZLOG    ") ||
             String::Equals(identifier, "ZPANDA  ") ||
             String::Equals(identifier, "WHICGRUB"))
    {
        _legacyType.enabledTrackPieces &= ~(1ULL << TRACK_SLOPE_STEEP);
    }
}

uint8 RideObject::CalculateNumVerticalFrames(const rct_ride_entry_vehicle * vehicleEntry)
{
    // 0x6DE90B
    uint8 numVerticalFrames;
    if (vehicleEntry->flags & VEHICLE_ENTRY_FLAG_OVERRIDE_NUM_VERTICAL_FRAMES)
    {
        numVerticalFrames = vehicleEntry->num_vertical_frames_override;
    }
    else
    {
        if (!(vehicleEntry->flags & VEHICLE_ENTRY_FLAG_SPINNING_ADDITIONAL_FRAMES))
        {
            if (vehicleEntry->flags & VEHICLE_ENTRY_FLAG_VEHICLE_ANIMATION && vehicleEntry->animation != VEHICLE_ENTRY_ANIMATION_OBSERVATION_TOWER)
            {
                if (!(vehicleEntry->flags & VEHICLE_ENTRY_FLAG_DODGEM_INUSE_LIGHTS))
                {
                    numVerticalFrames = 4;
                }
                else
                {
                    numVerticalFrames = 2;
                }
            }
            else
            {
                numVerticalFrames = 1;
            }
        }
        else
        {
            numVerticalFrames = 32;
        }
    }

    return numVerticalFrames;
}

uint8 RideObject::CalculateNumHorizontalFrames(const rct_ride_entry_vehicle * vehicleEntry)
{
    uint8 numHorizontalFrames;
    if (vehicleEntry->flags & VEHICLE_ENTRY_FLAG_SWINGING)
    {
        if (!(vehicleEntry->flags & VEHICLE_ENTRY_FLAG_21) && !(vehicleEntry->flags & VEHICLE_ENTRY_FLAG_SLIDE_SWING))
        {
            if (vehicleEntry->flags & VEHICLE_ENTRY_FLAG_25)
            {
                numHorizontalFrames = 3;
            }
            else
            {
                numHorizontalFrames = 5;
            }
        }
        else if (!(vehicleEntry->flags & VEHICLE_ENTRY_FLAG_21) || !(vehicleEntry->flags & VEHICLE_ENTRY_FLAG_SLIDE_SWING))
        {
            numHorizontalFrames = 7;
        }
        else
        {
            numHorizontalFrames = 13;
        }
    }
    else
    {
        numHorizontalFrames = 1;
    }

    return numHorizontalFrames;
}

static uint8 ParseRideType(const std::string &s)
{
    static const std::unordered_map<std::string, uint8> LookupTable
    {
        { "spiral_rc",                  RIDE_TYPE_SPIRAL_ROLLER_COASTER },
        { "stand_up_rc",                RIDE_TYPE_STAND_UP_ROLLER_COASTER },
        { "suspended_swinging_rc",      RIDE_TYPE_SUSPENDED_SWINGING_COASTER },
        { "inverted_rc",                RIDE_TYPE_INVERTED_ROLLER_COASTER },
        { "junior_rc",                  RIDE_TYPE_JUNIOR_ROLLER_COASTER },
        { "miniature_railway",          RIDE_TYPE_MINIATURE_RAILWAY },
        { "monorail",                   RIDE_TYPE_MONORAIL },
        { "mini_suspended_rc",          RIDE_TYPE_MINI_SUSPENDED_COASTER },
        { "boat_hire",                  RIDE_TYPE_BOAT_HIRE },
        { "wooden_wild_mouse",          RIDE_TYPE_WOODEN_WILD_MOUSE },
        { "steeplechase",               RIDE_TYPE_STEEPLECHASE },
        { "car_ride",                   RIDE_TYPE_CAR_RIDE },
        { "launched_freefall",          RIDE_TYPE_LAUNCHED_FREEFALL },
        { "bobsleigh_rc",               RIDE_TYPE_BOBSLEIGH_COASTER },
        { "observation_tower",          RIDE_TYPE_OBSERVATION_TOWER },
        { "looping_rc",                 RIDE_TYPE_LOOPING_ROLLER_COASTER },
        { "dinghy_slide",               RIDE_TYPE_DINGHY_SLIDE },
        { "mine_train_rc",              RIDE_TYPE_MINE_TRAIN_COASTER },
        { "chairlift",                  RIDE_TYPE_CHAIRLIFT },
        { "corkscrew_rc",               RIDE_TYPE_CORKSCREW_ROLLER_COASTER },
        { "maze",                       RIDE_TYPE_MAZE },
        { "spiral_slide",               RIDE_TYPE_SPIRAL_SLIDE },
        { "go_karts",                   RIDE_TYPE_GO_KARTS },
        { "log_flume",                  RIDE_TYPE_LOG_FLUME },
        { "river_rapids",               RIDE_TYPE_RIVER_RAPIDS },
        { "dodgems",                    RIDE_TYPE_DODGEMS },
        { "swinging_ship",              RIDE_TYPE_SWINGING_SHIP },
        { "swinging_inverter_ship",     RIDE_TYPE_SWINGING_INVERTER_SHIP },
        { "food_stall",                 RIDE_TYPE_FOOD_STALL },
        { "drink_stall",                RIDE_TYPE_DRINK_STALL },
        { "shop",                       RIDE_TYPE_SHOP },
        { "merry_go_round",             RIDE_TYPE_MERRY_GO_ROUND },
        { "information_kiosk",          RIDE_TYPE_INFORMATION_KIOSK },
        { "toilets",                    RIDE_TYPE_TOILETS },
        { "ferris_wheel",               RIDE_TYPE_FERRIS_WHEEL },
        { "motion_simulator",           RIDE_TYPE_MOTION_SIMULATOR },
        { "3d_cinema",                  RIDE_TYPE_3D_CINEMA },
        { "top_spin",                   RIDE_TYPE_TOP_SPIN },
        { "space_rings",                RIDE_TYPE_SPACE_RINGS },
        { "reverse_freefall_rc",        RIDE_TYPE_REVERSE_FREEFALL_COASTER },
        { "lift",                       RIDE_TYPE_LIFT },
        { "vertical_drop_rc",           RIDE_TYPE_VERTICAL_DROP_ROLLER_COASTER },
        { "cash_machine",               RIDE_TYPE_CASH_MACHINE },
        { "twist",                      RIDE_TYPE_TWIST },
        { "haunted_house",              RIDE_TYPE_HAUNTED_HOUSE },
        { "first_aid",                  RIDE_TYPE_FIRST_AID },
        { "circus",                     RIDE_TYPE_CIRCUS },
        { "ghost_train",                RIDE_TYPE_GHOST_TRAIN },
        { "twister_rc",                 RIDE_TYPE_TWISTER_ROLLER_COASTER },
        { "wooden_rc",                  RIDE_TYPE_WOODEN_ROLLER_COASTER },
        { "side_friction",              RIDE_TYPE_SIDE_FRICTION_ROLLER_COASTER },
        { "steel_wild_mouse",           RIDE_TYPE_STEEL_WILD_MOUSE },
        { "multi_dimension_rc",         RIDE_TYPE_MULTI_DIMENSION_ROLLER_COASTER },
        { "flying_rc",                  RIDE_TYPE_FLYING_ROLLER_COASTER },
        { "virginia_reel",              RIDE_TYPE_VIRGINIA_REEL },
        { "splash_boats",               RIDE_TYPE_SPLASH_BOATS },
        { "mini_helicopters",           RIDE_TYPE_MINI_HELICOPTERS },
        { "lay_down_rc",                RIDE_TYPE_LAY_DOWN_ROLLER_COASTER },
        { "suspended_monorail",         RIDE_TYPE_SUSPENDED_MONORAIL },
        { "reverser_rc",                RIDE_TYPE_REVERSER_ROLLER_COASTER },
        { "heartline_twister_rc",       RIDE_TYPE_HEARTLINE_TWISTER_COASTER },
        { "mini_golf",                  RIDE_TYPE_MINI_GOLF },
        { "giga_rc",                    RIDE_TYPE_GIGA_COASTER },
        { "roto_drop",                  RIDE_TYPE_ROTO_DROP },
        { "flying_saucers",             RIDE_TYPE_FLYING_SAUCERS },
        { "crooked_house",              RIDE_TYPE_CROOKED_HOUSE },
        { "monorail_cycles",            RIDE_TYPE_MONORAIL_CYCLES },
        { "compact_inverted",           RIDE_TYPE_COMPACT_INVERTED_COASTER },
        { "water_coaster",              RIDE_TYPE_WATER_COASTER },
        { "air_powered_vertical_rc",    RIDE_TYPE_AIR_POWERED_VERTICAL_COASTER },
        { "inverted_hairpin_rc",        RIDE_TYPE_INVERTED_HAIRPIN_COASTER },
        { "magic_carpet",               RIDE_TYPE_MAGIC_CARPET },
        { "submarine_ride",             RIDE_TYPE_SUBMARINE_RIDE },
        { "river_rafts",                RIDE_TYPE_RIVER_RAFTS },
        { "enterprise",                 RIDE_TYPE_ENTERPRISE },
        { "inverted_impulse_rc",        RIDE_TYPE_INVERTED_IMPULSE_COASTER },
        { "mini_rc",                    RIDE_TYPE_MINI_ROLLER_COASTER },
        { "mine_ride",                  RIDE_TYPE_MINE_RIDE },
        { "lim_launched_rc",            RIDE_TYPE_LIM_LAUNCHED_ROLLER_COASTER },

        // TEMPORARY:
        { "restroom",                   RIDE_TYPE_TOILETS },
    };
    auto result = LookupTable.find(s);
    return (result != LookupTable.end()) ?
        result->second :
        RIDE_TYPE_NULL;
}

static uint8 ParseRideCategory(const std::string &s)
{
    static const std::unordered_map<std::string, uint8> LookupTable
    {
        { "transport",      RIDE_CATEGORY_TRANSPORT },
        { "gentle",         RIDE_CATEGORY_GENTLE },
        { "rollercoaster",  RIDE_CATEGORY_ROLLERCOASTER },
        { "thrill",         RIDE_CATEGORY_THRILL },
        { "water",          RIDE_CATEGORY_WATER },
        { "stall",          RIDE_CATEGORY_SHOP },
    };
    auto result = LookupTable.find(s);
    return (result != LookupTable.end()) ?
        result->second :
        RIDE_CATEGORY_TRANSPORT;
}

static uint8 ParseShopItem(const std::string &s)
{
    static const std::unordered_map<std::string, uint8> LookupTable
    {
        { "burger",         SHOP_ITEM_BURGER },
        { "fries",          SHOP_ITEM_CHIPS },
        { "icecream",       SHOP_ITEM_ICE_CREAM },
        { "cottoncandy",    SHOP_ITEM_CANDYFLOSS },
        { "pizza",          SHOP_ITEM_PIZZA },
        { "popcorn",        SHOP_ITEM_POPCORN },
        { "hotdog",         SHOP_ITEM_HOT_DOG },
        { "seafood",        SHOP_ITEM_TENTACLE },
        { "candyapple",     SHOP_ITEM_TOFFEE_APPLE },
        { "donut",          SHOP_ITEM_DOUGHNUT },
        { "chicken",        SHOP_ITEM_CHICKEN },
        { "pretzel",        SHOP_ITEM_PRETZEL },
        { "funnelcake",     SHOP_ITEM_FUNNEL_CAKE },
        { "beefnoodles",    SHOP_ITEM_BEEF_NOODLES },
        { "friednoodles",   SHOP_ITEM_FRIED_RICE_NOODLES },
        { "wontonsoup",     SHOP_ITEM_WONTON_SOUP },
        { "meatballsoup",   SHOP_ITEM_MEATBALL_SOUP },
        { "subsandwich",    SHOP_ITEM_SUB_SANDWICH },
        { "cookies",        SHOP_ITEM_COOKIE },
        { "roastsausage",   SHOP_ITEM_ROAST_SAUSAGE },
        { "cola",           SHOP_ITEM_DRINK },
        { "coffee",         SHOP_ITEM_COFFEE },
        { "lemonade",       SHOP_ITEM_LEMONADE },
        { "hotchocolate",   SHOP_ITEM_CHOCOLATE },
        { "icedtea",        SHOP_ITEM_ICED_TEA },
        { "fruitjuice",     SHOP_ITEM_FRUIT_JUICE },
        { "soybeanmilk",    SHOP_ITEM_SOYBEAN_MILK },
        { "sujongkwa",      SHOP_ITEM_SU_JONGKWA },
        { "balloon",        SHOP_ITEM_BALLOON },
        { "plushtoy",       SHOP_ITEM_TOY },
        { "map",            SHOP_ITEM_MAP },
        { "onridephoto",    SHOP_ITEM_PHOTO },
        { "umbrella",       SHOP_ITEM_UMBRELLA },
        { "voucher",        SHOP_ITEM_VOUCHER },
        { "hat",            SHOP_ITEM_HAT },
        { "tshirt",         SHOP_ITEM_TSHIRT },
        { "sunglasses",     SHOP_ITEM_SUNGLASSES },
    };
    auto result = LookupTable.find(s);
    return (result != LookupTable.end()) ?
        result->second :
        SHOP_ITEM_NONE;
}

void RideObject::ReadJson(IReadObjectContext * context, const json_t * root)
{
    auto rideTypes = ObjectJsonHelpers::GetJsonStringArray(json_object_get(json_object_get(root, "properties"), "type"));
    for (size_t i = 0; i < std::min<size_t>(MAX_RIDE_TYPES_PER_RIDE_ENTRY, rideTypes.size()); i++)
    {
        _legacyType.ride_type[i] = ParseRideType(rideTypes[i]);
    }

    auto rideCategories = ObjectJsonHelpers::GetJsonStringArray(json_object_get(json_object_get(root, "properties"), "category"));
    for (size_t i = 0; i < std::min<size_t>(MAX_CATEGORIES_PER_RIDE, rideCategories.size()); i++)
    {
        _legacyType.category[0] = ParseRideCategory(rideCategories[i]);
    }

    // Shop item
    auto rideSells = ObjectJsonHelpers::GetJsonStringArray(json_object_get(json_object_get(root, "properties"), "sells"));
    _legacyType.shop_item = SHOP_ITEM_NONE;
    _legacyType.shop_item_secondary = SHOP_ITEM_NONE;
    if (rideSells.size() >= 1)
    {
        _legacyType.shop_item = ParseShopItem(rideSells[0]);
    }
    if (rideSells.size() >= 2)
    {
        _legacyType.shop_item_secondary = ParseShopItem(rideSells[1]);
    }

    auto stringTable = GetStringTable();
    auto jsonStrings = json_object_get(root, "strings");
    auto jsonName = json_object_get(jsonStrings, "name");
    auto jsonDescription = json_object_get(jsonStrings, "description");

    stringTable->SetString(0, 0, json_string_value(json_object_get(jsonName, "en-GB")));
    stringTable->SetString(1, 0, json_string_value(json_object_get(jsonDescription, "en-GB")));
    stringTable->SetString(2, 0, "Capacity");
    stringTable->SetString(3, 0, "Vehicle");

    auto imageTable = GetImageTable();
    ObjectJsonHelpers::LoadImages(root, *imageTable);

    rct_ride_entry_vehicle * vehicle0 = &_legacyType.vehicles[0];
    vehicle0->sprite_flags |= VEHICLE_SPRITE_FLAG_FLAT;
    vehicle0->base_image_id = 0;
}
