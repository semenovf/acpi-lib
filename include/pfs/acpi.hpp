////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2020 Vladislav Trifochkin
//
// This file is part of [pfs-acpi](https://github.com/semenovf/pfs-acpi) library.
//
// Changelog:
//      2020.04.10 Initial version
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include <memory>
#include <string>
#include <ostream>

namespace pfs {

enum class charge_state_enum
{
      unknown   //!< hardware doesn't give information about the state
    , charge    //!< battery is currently charging
    , discharge //!< battery is currently discharging
    , charged   //!< battery is charged
};

struct battery
{
    std::string name;
    std::string manufacturer;
    std::string model_name;
    std::string technology;
    charge_state_enum charge_state;
    int percentage;
    int seconds; // seconds until charged or remaining according `charge_state`
                 // or -1 if rate information unavailable
                 // or charging at zero rate or discharging at zero rate
};

enum class ac_state_enum
{
      unknown     //!< AC information unknown or not supported
    , offline     //!< AC adapter is off-line
    , online      //!< AC adapter is on-line
};

struct ac_adapter
{
    std::string name;
    ac_state_enum state;
};

struct thermal_zone
{
    std::string name;
    float temperature; // in degrees Celsius
};

struct fan
{
    std::string name;
    int cur_state;
    int max_state;
};

namespace details {
class acpi;
}

class acpi
{
public:
    enum device_enum {
          dev_none = 0
        , dev_battery      = 1 << 0
        , dev_ac_adapter   = 1 << 1
        , dev_thermal_zone = 1 << 2
        , dev_fan          = 1 << 3
        , dev_cooling = dev_fan
        , dev_all = dev_battery | dev_ac_adapter | dev_thermal_zone | dev_fan
    };

public:
    acpi ();
    ~acpi ();

    void acquire (int devices = dev_all);
    size_t batteries_available () const;
    size_t ac_adapters_available () const;
    size_t thermal_zones_available () const;
    size_t fans_available () const;
    battery battery_at (int index) const;
    ac_adapter ac_adapter_at (int index) const;
    thermal_zone thermal_zone_at (int index) const;
    fan fan_at (int index) const;

    void dump (std::ostream & out, bool extended_data = false);

    static bool has_acpi_support ();

private:
    std::unique_ptr<details::acpi> _d;
};

inline std::string to_string (ac_state_enum state)
{
    switch (state) {
        case ac_state_enum::offline:
            return "off-line";
        case ac_state_enum::online:
            return "on-line";
        default:
            break;
    }

    return "unknown";
}

inline std::string to_string (charge_state_enum state)
{
    switch (state) {
        case charge_state_enum::charge:
            return "charge";
        case charge_state_enum::discharge:
            return "discharge";
        case charge_state_enum::charged:
            return "charged";
        case charge_state_enum::unknown:
            break;
    }

    return "unknown";
}

} // namespace pfs
