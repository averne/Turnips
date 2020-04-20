#include <cstdio>
#include <cstdint>
#include <utility>
#include <switch.h>

#include "fs.hpp"
#include "turnip_parser.hpp"
#include "save.hpp"

constexpr auto acnh_programid = 0x01006f8002326000ul;
constexpr auto save_main_path = "/main.dat";
constexpr auto save_hdr_path  = "/mainHeader.dat";

extern "C" void userAppInit() {
#ifdef DEBUG
    socketInitializeDefault();
    nxlinkStdio();
#else
    consoleInit(nullptr);
#endif
}

extern "C" void userAppExit() {
#ifdef DEBUG
    socketExit();
#else
    consoleExit(nullptr);
#endif
}

#define PRINT(fmt, ...) ({      \
    printf(fmt, ##__VA_ARGS__); \
    consoleUpdate(nullptr);     \
})

int main(int argc, char **argv) {
    PRINT("Opening save...\n");
    FsFileSystem handle;
    auto rc = fsOpen_DeviceSaveData(&handle, acnh_programid);
    if (R_FAILED(rc))
        PRINT("Failed to open save: %#x\n", rc);
    auto fs = fs::Filesystem(handle);

    fs::File header, main;
    if (rc = fs.open_file(header, save_hdr_path) | fs.open_file(main, save_main_path); R_FAILED(rc))
        PRINT("Failed to open save files: %#x\n", rc);

    PRINT("Deriving keys...\n");
    auto [key, ctr] = sv::get_keys(header);
    PRINT("Decrypting save...\n");
    auto decrypted  = sv::decrypt(main, 0x500000, key, ctr);

    printf("Parsing save...\n");
    auto parser = tp::Parser(header, std::move(decrypted));
    auto p = parser.get_prices();

    printf("Buy price: %d\n", p.buy_price);
    printf("Sun: AM: %03d, PM: %03d\n", p.sunday_am_price,    p.sunday_pm_price);
    printf("Mon: AM: %03d, PM: %03d\n", p.monday_am_price,    p.monday_pm_price);
    printf("Tue: AM: %03d, PM: %03d\n", p.tuesday_am_price,   p.tuesday_pm_price);
    printf("Wed: AM: %03d, PM: %03d\n", p.wednesday_am_price, p.wednesday_pm_price);
    printf("Thu: AM: %03d, PM: %03d\n", p.thursday_am_price,  p.thursday_pm_price);
    printf("Fri: AM: %03d, PM: %03d\n", p.friday_am_price,    p.friday_pm_price);
    printf("Sat: AM: %03d, PM: %03d\n", p.satursday_am_price, p.satursday_pm_price);

    while (appletMainLoop()) {
        hidScanInput();
        if (hidKeysDown(CONTROLLER_P1_AUTO) & KEY_PLUS)
            break;
        consoleUpdate(nullptr);
    }

    header.close(), main.close();
    fs.close();

    return 0;
}
