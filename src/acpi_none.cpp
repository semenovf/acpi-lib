////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2020 Vladislav Trifochkin
//
// This file is part of [pfs-acpi](https://github.com/semenovf/pfs-acpi) library.
//
// Changelog:
//      2020.04.10 Initial version
////////////////////////////////////////////////////////////////////////////////
#include "pfs/acpi.hpp"

namespace pfs {

acpi::acpi ()
{}

acpi::~acpi ()
{}

bool acpi::has_acpi_support ()
{
    return false;
}

void acpi::acquire (int /*devices*/) const
{}

ac_state_enum acpi::ac_state () const
{
    return ac_state_enum::unsupported;
}

size_t acpi::batteries_available () const
{
    return 0;
}

size_t acpi::ac_adapters_available () const
{
    return 0;
}

size_t acpi::thermal_zones_available () const
{
    return 0;
}

size_t fans_available () const
{
    return 0;
}

battery acpi::battery_at (int /*index*/) const
{
    return battery{};
}

ac_adapter acpi::ac_adapter_at (int /*index*/) const
{
    return ac_adapter{};
}

thermal_zone thermal_zone_at (int /*index*/) const
{
    return thermal_zone{};
}

fan acpi::fan_at (int /*index*/) const
{
    return 0;
}

void acpi::dump (std::ostream & /*out*/, bool /*extended_data*/)
{}

} // namespace pfs

