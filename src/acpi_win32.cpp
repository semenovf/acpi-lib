////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2020 Vladislav Trifochkin
//
// This file is part of [pfs-acpi](https://github.com/semenovf/pfs-acpi) library.
//
// Changelog:
//      2020.04.12 Initial version
////////////////////////////////////////////////////////////////////////////////
#include "pfs/acpi.hpp"
#include <iomanip>
#include <vector>
#include <ostream>
#include <windows.h>

//
// [GetSystemPowerStatus function](https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-getsystempowerstatus)
//

namespace pfs {

namespace details {

struct battery_extended : battery
{};

class acpi
{
public:
    acpi ()
    {}

    void acquire_power_supply (int devices);
    void acquire_thermal (int devices);

    size_t batteries_available () const
    {
        return _batteries.size();
    }

    size_t ac_adapters_available () const
    {
        return _ac_adapters.size();
    }

    size_t thermal_zones_available () const
    {
        return _thermal_zones.size();
    }

    size_t fans_available () const
    {
        return _fans.size();
    }

    battery battery_at (int index) const
    {
        if (index >= 0 && index < _batteries.size()) {
            return _batteries[index];
        }
        return battery{};
    }

    ac_adapter ac_adapter_at (int index) const
    {
        if (index >= 0 && index < _ac_adapters.size()) {
            return _ac_adapters[index];
        }
        return ac_adapter{};
    }

    thermal_zone thermal_zone_at (int index) const
    {
        if (index >= 0 && index < _thermal_zones.size()) {
            return _thermal_zones[index];
        }
        return thermal_zone{};
    }

    fan fan_at (int index) const
    {
        if (index >= 0 && index < _fans.size()) {
            return _fans[index];
        }
        return fan{};
    }

    void dump (std::ostream & out, bool extended_data);

private:
    std::vector<battery_extended> _batteries;
    std::vector<ac_adapter>       _ac_adapters;
    std::vector<thermal_zone>     _thermal_zones;
    std::vector<fan>              _fans;
};

void acpi::acquire_power_supply (int devices)
{
    if (devices & pfs::acpi::dev_battery)
        _batteries.clear();

    if (devices & pfs::acpi::dev_ac_adapter)
        _ac_adapters.clear();

    SYSTEM_POWER_STATUS power_status;
    auto result = GetSystemPowerStatus(& power_status);

    // Error
    if (result == 0)
        return;

    if (devices & pfs::acpi::dev_battery) {
        _batteries.emplace_back();
        auto & bat = _batteries.back();
        bat.name = "unknown";
        bat.manufacturer = "unknown";
        bat.model_name = "unknown";
        bat.technology = "unknown";

        bat.percentage = power_status.BatteryLifePercent;
        bat.seconds = power_status.BatteryLifeTime;
    }

    if (devices & pfs::acpi::dev_ac_adapter) {
        _ac_adapters.emplace_back();
        auto & ac = _ac_adapters.back();
        ac.name = "unknown";
        ac.state = ac_state_enum::unknown;

        if (power_status.ACLineStatus == 0)
            ac.state = ac_state_enum::offline;
        else if (power_status.ACLineStatus == 1)
            ac.state = ac_state_enum::online;
    }
}

void acpi::acquire_thermal (int devices)
{
    if (devices & pfs::acpi::dev_thermal_zone)
        _thermal_zones.clear();

    if (devices & pfs::acpi::dev_fan)
        _fans.clear();

    // TODO Implement
}

void acpi::dump (std::ostream & out, bool extended_data)
{
    out << "Batteries available: " << batteries_available() << "\n";

    for (int i = 0; i < _batteries.size(); i++) {
        auto const & bat = _batteries[i];
        out << "Battery " << i << "\n";
        out << "\tname              : " << bat.name << "\n";
        out << "\tmanufacturer      : " << bat.manufacturer << "\n";
        out << "\tmodel name        : " << bat.model_name << "\n";
        out << "\ttechnology        : " << bat.technology << "\n";
        out << "\tstatus            : " << to_string(bat.charge_state) << "\n";
        out << "\tpercentage        : " << bat.percentage << "\n";
        out << "\tseconds           : " << bat.seconds << "\n";

        if (bat.seconds > 0) {
            auto seconds = bat.seconds;
            int hours = seconds / 3600;
            seconds -= 3600 * hours;
            int minutes = seconds / 60;
            seconds -= 60 * minutes;

            if (bat.charge_state == charge_state_enum::discharge)
                out << "\ttime remaining    : ";
            else
                out << "\ttime until charged: ";

            out << std::setw(2) << std::setfill('0') << hours
                    << ':' << std::setw(2) << std::setfill('0') << minutes
                    << ':' << std::setw(2) << std::setfill('0') << seconds
                    << "\n";
        }
    }

    out << "AC adapters available: " << ac_adapters_available() << "\n";

    for (int i = 0; i < _ac_adapters.size(); i++) {
        auto const & ac = _ac_adapters[i];
        out << "AC adapter " << i << "\n";
        out << "\tname  : " << ac.name << "\n";
        out << "\tstatus: " << to_string(ac.state) << "\n";
    }

    out << "Thermal zones available: " << thermal_zones_available() << "\n";

    for (int i = 0; i < _thermal_zones.size(); i++) {
        auto const & tz = _thermal_zones[i];
        out << "Thermal zone " << i << "\n";
        out << "\tname       : " << tz.name << "\n";
        out << "\ttemperature: " << tz.temperature << " degrees Celsius\n";
    }

    out << "Fans (cooling devices) available: " << fans_available() << "\n";

    for (int i = 0; i < _fans.size(); i++) {
        auto const & fan = _fans[i];
        out << "Fan (Cooling device) " << i << "\n";
        out << "\tname       : " << fan.name << "\n";
        out << "\tcur state  : " << fan.cur_state << "\n";
        out << "\tmax state  : " << fan.max_state << "\n";
    }
}

} // namespace details

acpi::acpi ()
{
    _d.reset(new details::acpi);
}

acpi::~acpi()
{}

bool acpi::has_acpi_support ()
{
    return true;
}

void acpi::acquire (int devices)
{
    // Acquire batteries and AC adapaters
    if ((devices & dev_battery) || (devices & dev_ac_adapter))
        _d->acquire_power_supply(devices);

    // Acquire thermal zones and fans
    if ((devices & dev_thermal_zone) || (devices & dev_fan))
        _d->acquire_thermal(devices);
}

size_t acpi::batteries_available () const
{
    return _d->batteries_available();
}

size_t acpi::ac_adapters_available () const
{
    return _d->ac_adapters_available();
}

size_t acpi::thermal_zones_available () const
{
    return _d->thermal_zones_available();
}

size_t acpi::fans_available () const
{
    return _d->fans_available();
}

battery acpi::battery_at (int index) const
{
    return _d->battery_at(index);
}

ac_adapter acpi::ac_adapter_at (int index) const
{
    return _d->ac_adapter_at(index);
}

thermal_zone acpi::thermal_zone_at (int index) const
{
    return _d->thermal_zone_at(index);
}

fan acpi::fan_at (int index) const
{
    return _d->fan_at(index);
}

void acpi::dump (std::ostream & out, bool extended_data)
{
    _d->dump(out, extended_data);
}

} // namespace pfs
