#include "pfs/acpi.hpp"
#include <iostream>

int main ()
{
    if (!pfs::acpi::has_acpi_support()) {
        std::cerr << "It's seems No ACPI support for your system!\n";
        return -1;
    }

    std::cout << "This system has ACPI support!\n";

    pfs::acpi acpi;
    acpi.acquire();

    std::cout << "AC state: " << to_string(acpi.ac_state()) << "\n";
    std::cout << "Batteries available: " << acpi.batteries_available() << "\n";

    return 0;
}
