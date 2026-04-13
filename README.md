# pico-z26 (Atari 2600 Emulator for Raspberry Pi Pico / Pico 2)

This project is a standalone **Atari 2600 emulator** running on
Raspberry Pi Pico, Pi Pico 2 and other RP2040/RP2350-based boards.
<br/>
[Original repo](https://www.whimsey.com/z26/)<br/>
[Best Hardware](https://murmulator.tilda.ws/)<br/>

It allows you to play classic Atari 2600 games directly on real
hardware, using SD card storage and external controllers.

------------------------------------------------------------------------

## Features

-   Play Atari 2600 games from SD card
-   Built-in file browser to select ROMs
-   Automatic loading of last played game from flash memory
-   On-device menu for settings and control
-   Support for standard Atari joystick controls
-   Support for USB gamepads
-   Support for USB and PS/2 keyboard
-   Basic audio output
-   Video output for HDMI and VGA displays; TV composite NTSC/PAL
-   Audio as PWM or i2s output

------------------------------------------------------------------------

## Game Loading

You can load games in two ways:

### From SD card

-   Insert SD card with ROM files
-   Open the file browser from the menu
-   Select a game to load

### From internal memory

-   The last selected game is saved
-   It will load automatically on next startup

------------------------------------------------------------------------

## Menu & Settings

The emulator includes a built-in menu system.

From the menu you can:
 - Select and load games
 - Reset the game
 - Change difficulty switches
 - Return to gameplay

------------------------------------------------------------------------

## Controls

Controls depend on the connected device.

### Joystick / Gamepad

-   Directional controls --- move
-   Button --- fire

### System controls

-   Reset --- restart game
-   Select --- change game mode (if supported)

### Menu access

-   Menu can be opened using a dedicated button or key (depends on
    controller)

------------------------------------------------------------------------

## Limitations

-   Paddle controllers are not supported
-   Some special peripherals (e.g. KidVid) are not fully supported
-   Audio quality is basic
-   No save/load state feature

------------------------------------------------------------------------

## Notes

-   Game compatibility depends on the cartridge type
-   Some games may require specific controller types
-   Performance and behavior may vary depending on the board and setup

------------------------------------------------------------------------

## License

Original z26 emulator license applies (GPL v2).
