////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2020 Vladislav Trifochkin
//
// This file is part of [pfs-acpi](https://github.com/semenovf/pfs-acpi) library.
//
// Changelog:
//      2020.04.10 Initial version
////////////////////////////////////////////////////////////////////////////////
#include "pfs/acpi.hpp"
#include <vector>
#include <cstdio>
#include <cstring>
#include <sys/types.h>
#include <dirent.h>
#include <strings.h>
#include <unistd.h>
#include <iomanip>

namespace pfs {

static char const * ACPI_PATH_SYS = "/sys/class/";
static size_t BUF_SZ = 64;
static double MIN_CAPACITY = double{0.01};
static double MIN_PRESENT_RATE = double{0.01};

namespace details {

struct battery_extended : battery
{
    int remaining_capacity;
    int remaining_energy;
    int present_rate;
    int last_capacity;
    int last_capacity_unit;

    int voltage;
};

class acpi
{
public:
    acpi ()
    {}

    void acquire_power_supply (int devices);
    void acquire_thermal (int devices);
    size_t batteries_available () const;
    size_t ac_adapters_available () const;
    size_t thermal_zones_available () const;
    size_t fans_available () const;
    battery battery_at (int index) const;
    ac_adapter ac_adapter_at (int index) const;
    thermal_zone thermal_zone_at (int index) const;
    fan fan_at (int index) const;

    void dump (std::ostream & out, bool extended_data);

private:
    std::vector<battery_extended> _batteries;
    std::vector<ac_adapter>       _ac_adapters;
    std::vector<thermal_zone>     _thermal_zones;
    std::vector<fan>              _fans;
};

static std::string read_all (char const * path, bool remove_trailing_nl = false)
{
    auto input = fopen(path, "r");

    if (!input)
        return std::string{};

    std::string result;
    char buf[BUF_SZ];
    int n = 0;

    while ((n = fread(buf, 1, BUF_SZ, input))) {
        if (remove_trailing_nl && n > 0 && n < BUF_SZ) {
            if (buf[n - 1] == '\n')
                --n;
        }

        result.append(buf, n);
    }

    fclose(input);
    return result;
}

static int unit_value (std::string const & s)
{
    int n = -1;
    sscanf(s.c_str(), "%d", & n);
    return n;
}

bool starts_with (char const * s, char const * prefix)
{
    while (*s && *prefix && *s++ == *prefix++)
        ;
    return *prefix == '\x0';
}

inline bool is_dir_entry (struct dirent * de)
{
    return !strcmp(de->d_name, ".") || !strcmp(de->d_name, "..");
}

template <typename Visitor>
void acquire_devices (char const * direntry, int devices, Visitor && visitor)
{
    if (chdir(ACPI_PATH_SYS) < 0)
        return;

    if (::chdir(direntry) < 0)
        return;

    auto d = ::opendir(".");

    if (!d)
        return;

    struct dirent * de;

    while ((de = ::readdir(d))) {
        if (is_dir_entry(de))
            continue;

        // Restore current directory
        chdir(ACPI_PATH_SYS);
        chdir(direntry);

        visitor(de->d_name, devices);
    }

    closedir(d);
}

void acpi::acquire_power_supply (int devices)
{
    acquire_devices("power_supply", devices, [this] (char const * direntry, int devices) {
        bool is_battery = false;
        bool is_ac_adapter = false;

        if (chdir(direntry) == 0) {
            auto type = read_all("type");

            if (strncasecmp(type.c_str(), "battery", 7) == 0)
                is_battery = true;
            else if (strncasecmp(type.c_str(), "mains", 5) == 0)
                is_ac_adapter = true;
        } else {
            return;
        }

        if (is_battery && (devices & pfs::acpi::dev_battery)) {
            _batteries.emplace_back();
            auto & bat = _batteries.back();
            bat.name = direntry;

            bat.manufacturer = read_all("manufacturer", true);
            bat.model_name = read_all("model_name", true);
            bat.technology = read_all("technology", true);

            auto charge_state = read_all("status", true);
            bat.charge_state = charge_state_enum::unknown;

            if (strncasecmp(charge_state.c_str(), "disch", 5) == 0)
                bat.charge_state = charge_state_enum::discharge;
            else if (strncasecmp (charge_state.c_str(), "full", 4) == 0)
                bat.charge_state = charge_state_enum::charged;
            else if (strncasecmp (charge_state.c_str(), "chargi", 6) == 0)
                bat.charge_state = charge_state_enum::charge;

            ////////////////////////////////////////////////////////////////
            auto remaining_capacity = read_all("charge_now", true);
            bat.remaining_capacity = -1;

            if (!remaining_capacity.empty())
                bat.remaining_capacity = unit_value(remaining_capacity) / 1000;

            ////////////////////////////////////////////////////////////////
            auto remaining_energy = read_all("energy_now", true);
            bat.remaining_energy = -1;

            if (!remaining_energy.empty())
                bat.remaining_energy = unit_value(remaining_energy) / 1000;

            ////////////////////////////////////////////////////////////////
            auto present_rate = read_all("current_now", true);
            bat.present_rate = -1;

            if (present_rate.empty())
                present_rate = read_all("power_now", true);

            if (!present_rate.empty())
                bat.present_rate = unit_value(present_rate) / 1000;

            ////////////////////////////////////////////////////////////////
            auto last_capacity = read_all("charge_full", true);
            bat.last_capacity = -1;

            if (!last_capacity.empty())
                bat.last_capacity = unit_value(last_capacity) / 1000;

            ////////////////////////////////////////////////////////////////
            auto last_capacity_unit = read_all("energy_full", true);
            bat.last_capacity_unit = -1;

            if (!last_capacity_unit.empty())
                bat.last_capacity_unit = unit_value(last_capacity_unit) / 1000;

            ////////////////////////////////////////////////////////////////
            auto voltage = read_all("voltage_now", true);
            bat.voltage = -1;

            if (!voltage.empty())
                bat.voltage = unit_value(voltage) / 1000;

            if (!bat.voltage)
                bat.voltage = -1;

            ////////////////////////////////////////////////////////////////
            // Recalculate attribute values
            ////////////////////////////////////////////////////////////////
            if (bat.last_capacity_unit != -1 && bat.last_capacity == -1) {
                if (bat.voltage != -1) {
                    bat.last_capacity = bat.last_capacity_unit * 1000 / bat.voltage;
                } else {
                    bat.last_capacity = bat.last_capacity_unit;
                }
            }

            if (bat.remaining_energy != -1 && bat.remaining_capacity == -1) {
                if (bat.voltage != -1) {
                    bat.remaining_capacity = bat.remaining_energy * 1000 / bat.voltage;
                    bat.present_rate = bat.present_rate * 1000 / bat.voltage;
                } else {
                    bat.remaining_capacity = bat.remaining_energy;
                }
            }

            if (bat.last_capacity < MIN_CAPACITY)
                bat.percentage = 0;
            else
                bat.percentage = bat.remaining_capacity * 100 / bat.last_capacity;

            if (bat.percentage > 100)
                bat.percentage = 100;

            bat.seconds = -1;

            if (bat.present_rate == -1) {
                bat.seconds = -1;
            } else if (bat.charge_state == charge_state_enum::charge) {
                if (bat.present_rate > MIN_PRESENT_RATE) {
                    bat.seconds = 3600 * (bat.last_capacity - bat.remaining_capacity) / bat.present_rate;
                } else {
                    bat.seconds = -1; // charging at zero rate
                }
            } else if (bat.charge_state == charge_state_enum::discharge) {
                if (bat.present_rate > MIN_PRESENT_RATE) {
                    bat.seconds = 3600 * bat.remaining_capacity / bat.present_rate;
                } else {
                    bat.seconds = -1; //discharging at zero rate
                }
            } else {
                bat.seconds = -1;
            }
        } else if (is_ac_adapter && (devices & pfs::acpi::dev_ac_adapter)) {
            _ac_adapters.emplace_back();
            auto & ac = _ac_adapters.back();
            ac.name = direntry;

            auto online = read_all("online", true);
            ac.state = ac_state_enum::unsupported;

            if (!online.empty()) {
                if (unit_value(online) == 0)
                    ac.state = ac_state_enum::offline;
                else
                    ac.state = ac_state_enum::online;
            }
        }
    });
}

void acpi::acquire_thermal (int devices)
{
    acquire_devices("thermal", devices, [this] (char const * direntry, int devices) {
        bool is_thermal_zone = false;
        bool is_fan = false;

        if (chdir(direntry) == 0) {
            auto temperature = read_all("temp");

            if (temperature.empty())
                is_fan = true;
            else
                is_thermal_zone = true;
        } else {
            return;
        }

        if (is_thermal_zone && (devices & pfs::acpi::dev_thermal_zone)) {
            _thermal_zones.emplace_back();
            auto & tz = _thermal_zones.back();
            tz.name = direntry;

            auto temperature = read_all("temp");
            tz.temperature = -1;

            if (!temperature.empty())
                tz.temperature = unit_value(temperature) / float{1000.0};
        } else if (is_fan && (devices & pfs::acpi::dev_fan)) {
            _fans.emplace_back();
            auto & fan = _fans.back();
            fan.name = direntry;

            auto cur_state = read_all("cur_state");
            fan.cur_state = -1;

            if (!cur_state.empty())
                fan.cur_state = unit_value(cur_state);

            auto max_state = read_all("max_state");
            fan.max_state = -1;

            if (!max_state.empty())
                fan.max_state = unit_value(max_state);

        }
    });
}

size_t acpi::batteries_available () const
{
    return _batteries.size();
}

size_t acpi::ac_adapters_available () const
{
    return _ac_adapters.size();
}

size_t acpi::thermal_zones_available () const
{
    return _thermal_zones.size();
}

size_t acpi::fans_available () const
{
    return _fans.size();
}

battery acpi::battery_at (int index) const
{
    if (index >= 0 && index < _batteries.size()) {
        return _batteries[index];
    }
    return battery{};
}

ac_adapter acpi::ac_adapter_at (int index) const
{
    if (index >= 0 && index < _ac_adapters.size()) {
        return _ac_adapters[index];
    }
    return ac_adapter{};
}

thermal_zone acpi::thermal_zone_at (int index) const
{
    if (index >= 0 && index < _thermal_zones.size()) {
        return _thermal_zones[index];
    }
    return thermal_zone{};
}

fan acpi::fan_at (int index) const
{
    if (index >= 0 && index < _fans.size()) {
        return _fans[index];
    }
    return fan{};
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

        if (extended_data) {
            out << "\tremaining capacity: " << bat.remaining_capacity << "\n";
            out << "\tremaining energy  : " << bat.remaining_energy << "\n";
            out << "\tpresent rate      : " << bat.present_rate << "\n";
            out << "\tlast_capacity     : " << bat.last_capacity << "\n";
            out << "\tlast_capacity_unit: " << bat.last_capacity_unit << "\n";
            out << "\tvoltage           : " << bat.voltage << "\n";
        }

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
    acquire();
}

acpi::~acpi()
{}

bool acpi::has_acpi_support ()
{
    return (chdir(ACPI_PATH_SYS) == 0);
}

int acpi::acpi_version ()
{
    // TODO Implement
    return 0;
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
