////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2020 Vladislav Trifochkin
//
// This file is part of [pfs-acpi](https://github.com/semenovf/pfs-acpi) library.
//
// Changelog:
//      2020.04.10 Initial version
////////////////////////////////////////////////////////////////////////////////
#pragma once

#if defined(__linux) || defined(__linux__)
#   define PFS_ACPI_IMPL_LINUX 1
extern "C" {
#   include "libacpi-0.2/libacpi.h"
}
#   include <cstring>
#endif

#include <string>

namespace pfs {

enum class ac_state_enum {
      unsupported //!< AC information not supported
    , offline     //!< AC adapter is off-line
    , online      //!< AC adapter is on-line
};

class acpi
{
public:
    acpi ();
    ~acpi ();

    void acquire ();
    int batteries_available () const;
    ac_state_enum ac_state () const;

    static bool has_acpi_support ();

private:
#if PFS_ACPI_IMPL_LINUX
    int _acstate {DISABLED};
    int _battstate {DISABLED};
    int _thermstate {DISABLED};
    int _fanstate {DISABLED};

    global_t _global;
//     battery_t * binfo {nullptr};
//     adapter_t * ac {nullptr} = & global->adapt;
//     thermal_t * tp {nullptr};
//     fan_t * fa {nullptr};

#endif
};

#if PFS_ACPI_IMPL_LINUX
    acpi::acpi ()
    {
        acquire();
    }

    acpi::~acpi()
    {}

    inline bool acpi::has_acpi_support ()
    {
        return (check_acpi_support() == NOT_SUPPORTED);
    }

    inline void acpi::acquire ()
    {
        std::memset(& _global, 0, sizeof(_global));

        // Initialize battery, thermal zones, fans and ac state
        _battstate = init_acpi_batt(& _global);     // Batteries
        _thermstate = init_acpi_thermal(& _global); // Power adapter
        _fanstate = init_acpi_fan(& _global);       // Fans
        _acstate = init_acpi_acadapt(& _global);    // Thermal zones

        // Read batteries values
        for (int i = 0; i < _global.batt_count; i++) {
            read_acpi_batt(i);
        }
    }

    inline ac_state_enum acpi::ac_state () const
    {
        adapter_t const * ac = & _global.adapt;

        if (_acstate == SUCCESS && ac->ac_state == P_BATT)
            return ac_state_enum::offline;
        else if (_acstate == SUCCESS && ac->ac_state == P_AC)
            return ac_state_enum::online;
        else
            return ac_state_enum::unsupported;
    }

    inline int acpi::batteries_available () const
    {
        return _global.batt_count;
    }

#else
    acpi::acpi ()
    {}

    acpi::~acpi()
    {}

    inline bool acpi::has_acpi_support ()
    {
        return false;
    }

    inline void acpi::acquire ()
    {}

    inline ac_state_enum acpi::ac_state () const
    {
        return ac_state_enum::unsupported;
    }

    inline int batteries_available () const
    {
        return 0;
    }
#endif

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

        return "not supported";
    }

} // namespace pfs
