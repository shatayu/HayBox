# HayBox Limited Edition

This is a firmware designed for experimenting with travel time and other subframe nerfs for Melee based on Haystack's HayBox.

We believe that digital controllers, when used for Melee, are a little overtuned relative to GCC.

Our goal is to make it so that box users and GCC users have a level playing field without doing this by buffing custom motherboard GCCs to the level of boxes, so that the barrier to entry for new players isn't a PhobGCC or a rectangle, or by changing the game to be significantly easier than it currently is.

Particular strengths of rectangle controllers we had in mind when designing the initial set of nerfs:

* Dashdancing: GCC players perceive that it's more difficult to hit a dashdancing box player because of their increased ability to react to a threat and dash away.
* SDI: While GCCs can produce tremendous sustained SDI using the "wank" technique, boxes have much more consistency for short SDI windows and we want to reduce their ability to produce many SDI inputs in a very short time.
  * Note: it will probably never be possible to make it more difficult to do two consecutive SDI inputs on a box.
* Pivot tilts: While empty pivoting is somewhat less consistent on rectangles, they are able to pinpoint tilt inputs after an empty pivot much more quickly and consistently than a GCC player can reasonably do without overshooting.
* Crouch uptilts: Reactionary crouching uptilts and run cancel uptilts, both strong options, are significantly easier on rectangles than on stick because a fast stick motion is likely to overshoot and result in an upsmash or tap jump.

Current nerfs (**SUBJECT TO CHANGE**)

* Neutral SOCD
  * Neutral SOCD helps mitigate easy three-consecutive-input SDI, though not completely.
  * Neutral SOCD also helps make long-distance dashdancing a little riskier, because there are two motions required to turn around when you want to change direction. Thus you have an increased risk of getting stuck in a skidding turnaround when you try to turn around right at the end of initial dash. Alternatively, you can release the forward direction early, but that sacrifices distance.
  * Many consider this to be slightly better than 2ip no-reactivation for controlling aerial drift, because you can never lock yourself out of drift.
  * It affects away-then-in ledgedashes, making them much harder to be frame-perfect similar to on sticks.
    * However, it is pointed out that c-stick ledgedrops still exist, just people would need to practice them (and they may be less ergonomic on rectangles with large button spacing).
    * Some players, depending on the character, want wider ModX wavedash angles to make held-modX down-then-in ledgedashes have the same timing. Is this just Fox?
  * However, it is generally riskier for vertical recovery mixups like fastfall up-b.
  * Additionally, if you're not used to lifting fingers up it may take time to get used to.
  * Will we stick with this? There are potential alternatives that have been suggested, though they are significantly harder to implement:
    * 2ip no reactivation that has significantly increased travel time when both directions are pressed
    * 2ip no reactivation vertically, but not horizontally. (possibly with the above); this would need an additional SDI nerf similar to the B0XX ones.
* Left stick travel time to the center or diagonal rim coordinates: 6ms (just over 1/3 frame)
  * Travel time exists to add some reaction time parity between sticks and buttons, and to also make digital inputs subject to at least some of the same polling timing considerations as sticks.
  * This value is based off measured center-to-rim stick speeds from various players, as well as a little bit of rectangle testing before where a player remarked that even slightly longer travel time felt worse.
  * Because the travel is linear, this delays inputs by 3 ms on average, but it may be slightly more depending on where the threshold is along the travel distance. For dashdancing turnarounds, this is more like a 5ms delay, but for dashing from standing it's more like 2ms.
* ~~Left stick movements to rim coordinates delayed by 4ms (1/4 frame).~~
  * This removes the risk of polling errors for rising edges but slightly slows reaction time.
* Left stick travel time to ~~non-center~~ non-cardinal or diagonal rim coordinates including the center: 12ms (2/3 frame)
  * This is longer than the rim travel times because it takes longer, on a stick, to pinpoint a non-rim position.
  * This used to be 16 ms, a whole frame, but users noted that it made timing button inputs for tilts harder. On a stick, even if it takes longer to get to a tilt location, you get feedback from your senses of when the motion is completed, but that's not true of button presses with simulated travel time.
* 4 frame travel time for cardinal tapping SDI (numpad notation: 5656...) faster than 10 presses/s, but not for taps shorter than half a frame for switch bounce leniency.
  * This is implemented because with neutral SOCD it's relatively easy to get double-rate mashing. To mash right with one full cycle per press AND per release, press right, press left, release left, release right (and repeat).
  * By merely increasing the travel time and not locking out, this mitigates SDI without significantly affecting aerial drift control via rapid tapping.
  * We need to do SDI testing to check whether this is needed at all for short SDI windows (Fox upair 1) and whether it can achieve more distance than wank SDI on long SDI windows (knee).
* ~~Lockout for diagonal tapping SDI (numpad notation: 636**3**...) faster than 10 presses/s (6f)~~ Currently Disabled
  * This was implemented for the same reasons as cardinal tap SDI.
  * However, at the moment it's disabled because it affects shield drop fastfall.
  * We would like users to test whether abusing diagonal tap SDI is noticeably impactful in game.
  * In the future this may be re-enabled with a different time window.
* Lockout for cardinal + diagonal tapping SDI (numpad notation: 56356**3**) faster than 7.5 presses/s (5.5f)
  * The last remaining SDI method gets you a cardinal and a diagonal SDI pulse for every press, by plinking two adjacent cardinals.
  * This is basically repeated quarter-circle SDI in a way that's not really doable on a stick.
  * It is occasionally impactful in game: one person reported that their ledgedash technique, which involves dropping from ledge with down then in, releasing, and pressing diagonal again to airdodge, was impacted.
* Lockout for cardinal-diagonal-opposite diagonal (numpad notation: 563**9**) with less than 5.5 frames between cardinal and second diagonal.
  * Even with neutral SOCD it's not too hard to get three consecutive SDI inputs without this.
* Moving from a crouching coordinate to an upward tilt in < 3 frames will move the stick to a tap jump coordinate
  * This prevents users from continuously holding crouch and uptilting on reaction in a single motion.
  * Additionally, it affects the speed of run cancel uptilt.
  * Neutral SOCD does not compound with this: time spent in neutral counts towards the two frames you have to wait.
  * This is a fairly non-controversial new nerf, because it's something that is a strong option in game that is extremely difficult to execute on a stick.
  * The 3 frame window was initially determined by asking top GCC controller modders what they thought would be a reasonable limit.
  * In early testing, players have said that it feels reasonable. The limit is easy enough to avoid but at the same time holds back degenerate options.
  * NOTE: earlier this said 2 frames. I checked the code and it had been 3 frames all along.
* Tilt stick inputs less than 8.0 frames after an empty pivot will move the stick to a smash coordinate.
  * This detects an empty pivot that is >50% probability of success: a 0.5 to 1.5 frame tap opposite of the last direction pressed.
  * Any upward or downward tilt input becomes full magnitude, as long as < 8 frames have elapsed since the empty pivot.
  * This corresponds with the difficulty of controlling the stick after an empty pivot movement.
  * For upward directions, a minimum angle of 45 degrees is enforced to prevent up-angled ftilt and produce a tap jump regardless of the original angle. Otherwise, it attempts to preserve the angle so wavedashes are not randomly affected.

In addition to nerfs, it offers several features:

Arduino-based boxes will work at the same latency regardless of polling rate, as long as the poll spacing is constant.
Users no longer need to hold A on plugin to optimize latency on console, and it'll have slightly less lag on adapters.

Instead, the A press on plugin disables all timing based nerfs.

Additionally, you can hold B on plugin to get the shorter recovery coordinates that all have magnitudes about 0.8, for use with teleport recovery characters, and you can hold Down on plugin to get the crouch-walk option select.

It also currently has built-in support for a handful more boards than mainline Haybox, notably B0XX R4, Htangl, and Rana Digital.

Limitations: Any console polling that does not have uniform space between polls will trip up the connection. Notably, the Homebrew Menu, Nintendont, SmashScope on console, and third-party GCC to USB adapters can work either poorly or not at all.

# HayBox

HayBox is a modular, cross-platform firmware for digital or mixed analog/digital controllers, primarily targeted at [B0XX](https://b0xx.com)-style controllers.

[![GitHub issues](https://img.shields.io/github/issues/JonnyHaystack/HayBox)](https://github.com/JonnyHaystack/HayBox/issues)
[![GitHub pull requests](https://img.shields.io/github/issues-pr/JonnyHaystack/HayBox)](https://github.com/JonnyHaystack/HayBox/pulls)

## Table of Contents

* [Features](#features)
* [Getting Started](#getting-started)
  * [Requirements](#requirements)
  * [Installation](#installation)
* [Usage](#usage)
  * [Default button holds](#default-button-holds)
  * [Dolphin setup](#dolphin-setup)
* [Customisation](#customisation)
  * [Console/gamemode selection bindings](#consolegamemode-selection-bindings)
  * [Creating custom input modes](#creating-custom-input-modes)
  * [Mod X lightshield and R shield tilt](#mod-x-lightshield-and-r-shield-tilt)
  * [Mode-specific optional features](#mode-specific-optional-features)
    * [Melee modes](#melee-modes)
    * [Project M/Project+ mode](#project-mproject-mode)
  * [Input sources](#input-sources)
  * [Using the Pico's second core](#using-the-picos-second-core)
* [Troubleshooting](#troubleshooting)
* [Contributing](#contributing)
* [Contributors](#contributors)
* [License](#license)

## Features

Features include:
- Cross platform support:
  - RP2040 (e.g. Raspberry Pi Pico)
  - 16MHz AVR MCUs (e.g. ATMega32U4 which several Arduinos are based on)
- Supports many existing controllers/PCBs, e.g. B0XX, LBX, Smash Box, Crane's
  GCCPCB/Model S
- Supports a variety of communication backends which can be used either separately or in conjunction with each other:
  - XInput
  - DInput
  - GameCube console
  - Nintendo 64 console
  - Nintendo Switch console
  - B0XX input viewer
- Supports a variety of "input sources" which can be used in conjunction to create mixed input controllers:
  - Buttons/switches wired directly to GPIO pins
  - Switch matrix (as typically found in keyboards)
  - Wii Nunchuk
  - GameCube controller
- Melee mode up to date with B0XX V3 specifications
- Existing modes for popular games (e.g. Melee, Project M, Ultimate, Rivals of Aether, traditional fighting games)
- Easy to create new controller modes (or keyboard modes) for different games
- USB keyboard game modes for games that lack gamepad support
- Fully customisable SOCD cleaning, allowing you to configure SOCD button pairs (e.g. left/right, up/down) for each controller/keyboard mode, and also easily change the SOCD resolution method for each SOCD pair
- Switch modes on the fly without unplugging your controller
- Automatically detects whether plugged into console or USB
- Game modes and communication backends are independent entities, meaning you can use any game mode with any supported console without extra work
- Easily switch between different GameCube/N64 polling rates in order to have optimal latency on console, overclocked adapter, etc. (not necessary for Pico/RP2040)

## Getting Started

### Requirements

- [Git](https://git-scm.com/downloads) - required only if you are using a Pico. May become unnecessary at some point in the future.
- [PlatformIO IDE for VSCode](https://platformio.org/install/ide?install=vscode)

### Installation

Download and extract the
[latest HayBox release](https://github.com/JonnyHaystack/HayBox/releases),
or clone the repository if you have git installed (which makes it easier for you
to pull updates).

After that:
1. Open Visual Studio Code
2. Click File -> **Open Folder** and choose the HayBox folder (the one containing platformio.ini, not the folder above that)
3. Choose the appropriate build environment for your controller's PCB by
  clicking the environment selection button near the bottom left of the window

  ![image](https://user-images.githubusercontent.com/1266473/187039372-485c5f0d-60b3-4534-befb-e713f138a7c8.png)
  ![image](https://user-images.githubusercontent.com/1266473/187039585-cea18994-fd12-45fb-b43f-427eb7affb81.png)

4. If your controller has a different pinout than any of the existing configs, you may edit the button mappings and other pins at the top of the config (`config/<environment>/config.cpp`). Any buttons that your controller doesn't have can simply be deleted from the list.
5. If you see a message in the bottom bar saying "Rebuilding IntelliSense Index" or "Loading Project Tasks", wait for it to disappear. For Pico especially it may take quite a while the first time because it has to download 2-3GB of dependencies.
6. Click **Build** (in the bottom left) and make sure everything compiles without
  errors
7. This  next step differs depending on the microcontroller used in your controller.
    - **For Pico-based controllers**: hold the bootsel button while plugging it in (or your Start button if you already have HayBox installed) and then drag and drop the file `HayBox/.pio/build/<environment>/firmware.uf2` onto the RPI-RP2 drive that comes up.
    - **For Arduino-based controllers**: Plug in your controller via USB and click **Upload** (next to the Build button)

## Usage

### Default button holds

#### Pico bootsel mode

To reboot Pico-based controllers into bootsel mode, hold Start on plugin.

#### Brook board passthrough mode

To switch to Brook board mode on GCCPCB2, GCCMX, B0XX R2, or LBX, hold B and R on
plugin.

#### Communication backends (console selection)

Communication backends are selected slightly differently depending on the type
of microcontroller used in the controller.

On Pico/RP2040, USB vs GameCube vs Nintendo 64 is detected automatically. If
not plugged into a console, the default is **XInput**, which should work
plug-and-play with most PC games.
Other backends are selected by holding one of the following buttons on plugin:
- X - Nintendo Switch USB mode (also sets initial game mode to Ultimate mode)
- Z - DInput mode (only recommended for games which don't support XInput)

On Arduino/AVR, the **DInput** backend is selected if a USB connection is detected.
Otherwise, it defaults to GameCube backend, unless another backend is manually
selected by holding one of the following buttons on plugin:
- A - GameCube backend with polling rate fix disabled (used for GCC adapters)
- C-Left - Nintendo 64 backend (60Hz polling rate)

#### Game mode selection

Unlike other similar firmwares, HayBox by default allows you to switch
modes on the fly without unplugging your controller. This is mainly useful on
PC, as opposed to console where you usually have to restart the console to
switch game anyway. It also serves the purpose of reducing the number of buttons
you have to hold with one hand while plugging in.

The default controller mode button combinations are:
- Mod X + Start + L - Melee mode (default)
- Mod X + Start + Left - Project M/Project+ mode
- Mod X + Start + Down - Ultimate mode
- Mod X + Start + Right - FGC mode (Hitbox style fighting game layout)
- Mod X + Start + B - Rivals of Aether mode

Default keyboard mode button combinations (only available when using DInput backend, **not** with XInput):
- Mod Y + Start + L - Default keyboard mode

### Dolphin setup

HayBox needs a different Dolphin controller profile than the official B0XX firmware, as it
uses different DInput mappings that make more sense for use across multiple games. These
can be found in the `dolphin` folder in the HayBox repo. The profile files are named to
indicate what communication backend and operating system they are for:
- For Windows:
  - HayBox_XInput.ini - For Pico/RP2040-based controllers (e.g. B0XX R4)
  - HayBox_DInput.ini - For Arduino/AVR-based controllers (e.g. B0XX R1-3, LBX)
- For Linux:
  - HayBox_XInput_Linux.ini - For Pico/RP2040-based controllers (e.g. B0XX R4)
  - HayBox_DInput_Linux.ini - For Arduino/AVR-based controllers (e.g. B0XX R1-3, LBX)
- For macOS (unsupported*):
  - HayBox_DInput_macOS.ini

To install the profile:
1. Copy the appropriate .ini file from the `dolphin` folder within HayBox to the folder `<YourDolphinInstallation>\User\Config\Profiles\GCPad\` (create it if it does not exist)
- For Slippi this should be
  - On Windows: `%appdata%\Slippi Launcher\netplay\User\Config\Profiles\GCPad\` (if this doesn't work, try `...\netplay\Sys\User\...`)
  - On Linux: `~/.config/SlippiOnline/Config/Profiles/GCPad/`
  - On Mac: `Cmd + Shift + G` and enter the path `/Users/<USER>/Library/Application Support/Slippi Launcher/netplay/Slippi Dolphin.app/Contents/Resources/User/Config/Profiles/GCPad`
- For vanilla Dolphin:
  - On Windows: `%userprofile%\Documents\Dolphin Emulator\Config\Profiles\GCPad\`
  - On Linux: `~/.config/dolphin-emu/Profiles/GCPad/`
2. Plug in your controller, and configure a "Standard Controller" in Dolphin
3. Click **Refresh** next to the Device dropdown
4. Select the HayBox profile from the profile dropdown, and click **Load** (NOT Save)
5. Make sure the correct device is selected in the device dropdown
6. Click **Close**

\* macOS only supports DInput (and not very well), so if using a Pico/RP2040-based controller you will have to force DInput mode by holding Z on plugin, and even then it may not work. I can't really do anything about Apple's poor controller support (which they seem to break with every other update) and I don't own any Apple devices, so this will also be considered unsupported usage of HayBox.

## Customisation

### Console/gamemode selection bindings

#### Communication backends (console selection)

The communication backend (e.g. DInput, GameCube, or N64) is selected partly
through auto detection and partly based on the buttons held on plugin. This is
handled in `config/<environment>/config.cpp`, in the `setup()` function.
The logic is fairly simple, and even if you don't have programming experience it
shouldn't be too hard to see what's going on and change things if you wish.

The config folders corresponding to the Arduino environments are:
- `config/arduino_nativeusb/` for Arduino with native USB support (e.g. Leonardo, Micro)
- `config/arduino/` for Arduino without native USB support (e.g. Uno, Nano, Mega 2560)

For Arduino device configs you may notice that the number 125 is passed into
`GamecubeBackend()`. This lets you change the polling rate e.g. if your DIY
doesn't support native USB and you want to use it with an overclocked GameCube
controller adapter. In that example, you could pass in 1000 to sync up to the
1000Hz polling rate, or 0 to disable this lag fix completely.
Polling rate can be passed into the N64Backend constructor in the same way as this.

You may notice that 1000Hz polling rate works on console as well. Be aware
that while this works, it will result in more input lag. The point of setting
the polling rate here is so that the GameCube backend can delay until right
before the next poll before reading the inputs, so that the inputs are fresh and
not outdated.

For Pico/RP2040, it is not necessary to pass in a console polling rate, because
the Pico has enough processing power to read/process inputs after receiving the
console poll, so there is no need to predict when the poll will arrive and
prepare things in advance.

#### Input modes

To configure the button holds for input modes (controller/keyboard modes), edit
the `select_mode()` function in `config/mode_selection.hpp`. Each `if`
statement is a button combination to select an input mode.

Most input modes support passing in an SOCD cleaning mode, e.g.
`socd::2IP_NO_REAC`. See [here](#socd) for the other available modes.

### Creating custom input modes

For creating new input modes, it helps if you know some C++, or at least have
some programming experience. That said, you should be able to get by even
without prior experience if you just base your new mode off the existing ones
and refer to them as examples.

There are two types of input modes: ControllerMode and KeyboardMode

#### Keyboard modes

Keyboard modes are a little bit simpler so let's start there.

A KeyboardMode behaves as a standard keyboard and should work with any device
that supports keyboards.

You are free to use whatever logic and programming tricks you like in the
`UpdateKeys()` function to decide the outputs based on the input state. You could
create input layers (like the D-Pad layer in Melee mode that is activated when
holding Mod X and Mod Y), or other types of conditional inputs.

The list of available keycodes can be found [here](https://github.com/JonnyHaystack/ArduinoKeyboard/blob/master/include/keycodes.h).

Remember that keyboard modes can only be activated when using the **DInput** communication backend (**not** XInput).

#### Controller modes

A ControllerMode takes a digital button input state and transforms it into an
output state corresponding to a standard gamepad. Any ControllerMode will work
with any CommunicationBackend. A CommunicationBackend simply reads inputs from one or more input sources, uses the current ControllerMode to update the outputs based on those inputs, and handles the sending of the outputs to the console or PC.

To create a ControllerMode, you just need to implement the functions
`UpdateDigitalOutputs()` and `UpdateAnalogOutputs()`.

`UpdateDigitalOutputs()` is very similar to the `UpdateKeys()` function in keyboard
modes, with the difference that rather than calling a `Press()` function to
immediately send inputs, we are simply setting the output state for this
iteration. As the name indicates, we will only deal with the digital outputs in
this function.

`UpdateAnalogOutputs()` is a bit more complicated. Firstly, it has to call
`UpdateDirections()` before doing anything else. This function takes in values
indicating whether your left and right sticks are pointing left/right/up/down.
You also pass in the minimum, neutral (centre), and maximum stick analog values,
so you can configure these on a per-mode basis. All this information is used to
automatically set the stick analog values based on the inputs you passed in. This
is all you need to do unless you want to implement modifiers.

`UpdateDirections()` also populates the variable `directions` with values
indicating current stick direction, which can be 1, 0, or -1 for the X and Y
axes for both sticks. These values make it much easier to write modifier logic.

After calling `UpdateDirections()`, add any modifier handling logic that you want.
Remember that `UpdateDirections()` already set the default analog stick values,
so when handling modifiers you only need to manually set the values for the axes
that are actually being modified. Other than this, I can't teach how to write
your modifier logic, so just look at the examples and play around.

Finally, set any analog trigger values that you need.

Note: Analog trigger outputs could just as well be handled in
`UpdateDigitalOutputs()`, but I think it usually looks cleaner to keep them
along with the other analog outputs.

Also note: You don't need to worry about resetting the output state or clearing
anything from it. This is done automatically at the start of each iteration.

### SOCD

In the constructor of each mode (for controller modes *and* keyboard modes), you
can configure pairs of opposing direction inputs to apply SOCD cleaning to.

For example, in `src/modes/Melee20Button.cpp`:
```
_socd_pair_count = 4;
_socd_pairs = new socd::SocdPair[_socd_pair_count]{
    socd::SocdPair{&InputState::left,    &InputState::right,   socd_type},
    socd::SocdPair{ &InputState::down,   &InputState::up,      socd_type},
    socd::SocdPair{ &InputState::c_left, &InputState::c_right, socd_type},
    socd::SocdPair{ &InputState::c_down, &InputState::c_up,    socd_type},
};
```

This sets up left/right, down/up, C-Left/C-Right, and C-Down/C-Up as pairs of
opposing cardinal directions for which SOCD cleaning will be applied. The SOCD
cleaning is automatically done before `UpdateDigitalOutputs()` and
`UpdateAnalogOutputs()`, and you do not need to worry about it any further than
that.

For each `SocdPair` you can pass in an `SocdType` of your choosing. By default
for most modes this is passed in as a single constructor parameter, but it is
possible to pass in multiple parameters, or simply use a hardcoded value. Both
of these approaches are exemplified in `src/modes/FgcMode.cpp`.

| `SocdType` | Description |
| ---------- | ----------- |
| `SOCD_NEUTRAL` | Left + right = neutral - the default if no `SocdType` specified in the `SocdPair` |
| `SOCD_2IP` | Second input priority - left -> left + right = right, and vice versa. Releasing the second direction gives the original direction |
| `SOCD_2IP_NO_REAC` | Second input priority without reactivation - same as above, except releasing the second direction results in neutral. The original direction must be physically reactuated. |
| `SOCD_DIR1_PRIORITY` | The first button in the `SocdPair` always takes priority over the second |
| `SOCD_DIR2_PRIORITY` | The second button in the `SocdPair` always takes priority over the first |
| `SOCD_NONE` | No SOCD resolution - the game decides |

Note that you do not have to implement a `HandleSocd()` function like in the
Melee20Button and Melee18Button modes. It is only overridden in these modes
so that we can check if left and right are both held *before* SOCD cleaning,
because when they are both held (without a vertical direction being held) we
need to override all modifiers.

### Mod X lightshield and R shield tilt

If your controller has no lightshield buttons, you may want to use Mod X for
lightshield and put shield tilt on R instead. You can do this by using the
Melee18Button mode instead of Melee20Button.

### Mode-specific optional features

#### Melee modes

The Melee20Button and Melee18Button modes provide a choice of which coordinates to use
when pressing down + right. By default, holding down + back will allow you to do automatic
jab-cancels, which is a useful technique for some characters.

Another popular technique that uses the down + right diagonal is the so-called crouch/walk
option-select. This technique involves holding down + forward at a certain angle while
crouching, such that after crouch-cancelling an attack you will automatically start
walking towards your opponent instead of going back into crouch. This can be very useful
for tech-chasing, but the coordinates used for this technique do not allow you to auto
jab-cancel.

This can be configured as seen in `config/mode_selection.hpp` by setting the `crouch_walk_os` option to true:

```
new Melee20Button(socd::SOCD_2IP_NO_REAC, { .crouch_walk_os = false })
```

You will also have to change this in your `config/<environment>/config.cpp` in order for it to be applied on plugin, as `mode_selection.hpp` only controls what happens when you *switch* mode.

#### Project M/Project+ mode

The ProjectM mode has some extra options to configure certain behaviours. As seen
in `config/mode_selection.hpp`:

```
new ProjectM(
    socd::SOCD_2IP_NO_REAC,
    { .true_z_press = false, .ledgedash_max_jump_traj = true }
)
```

Firstly, the `ledgedash_max_jump_traj` option allows you to enable or disable the behaviour
borrowed from Melee mode where holding left and right (and no vertical directions)
will give a 1.0 cardinal regardless of modifiers being held.

If you change the SOCD mode to 2IP (with reactivation), you should also change
this option to false if you want a smooth gameplay experience.

Secondly, the `true_z_press` option exists because Project M/Project+ do not handle
Z presses the same way Melee does.
Melee interprets a Z press as lightshield + A, and thus it can be used for L
cancelling without locking you out of techs. In PM/P+, a Z press will trigger a
tech and thus cause unwanted tech lockouts if used to L cancel.
By default in HayBox, the ProjectM mode is set to use a macro of lightshield + A
in order to preserve expected behaviour from Melee. However, this macro does not
enable you to use tether/grapple attacks or grab items. To workaround this, you
can press Mod X + Z to send a true Z input.

If this bothers you, and you just want to send a true Z input by default when
pressing Z, you can set the `true_z_press` option to true.

### Input sources

HayBox supports several input sources that can be read from to update the input
state:
- `GpioButtonInput` - The most commonly used, for reading switches/buttons connected directly to GPIO pins. The input mappings are defined by an array of `GpioButtonMapping` as can be seen in almost all existing configs.
- `SwitchMatrixInput` - Similar to the above, but scans a keyboard style switch matrix instead of individual switches. A config for Crane's Model C<=53 is included at `config/c53/config.cpp` which serves as an example of how to define and use a switch matrix input source.
- `NunchukInput` - Reads inputs from a Wii Nunchuk using i2c. This can be used for mixed input controllers (e.g. left hand uses a Nunchuk for movement, and right hand uses buttons for other controls)
- `GamecubeControllerInput` - Similar to the above, but reads from a GameCube controller. Can be instantiated similarly to GamecubeBackend. Currently only implemented for Pico, and you must either run it on a different pio instance (pio0 or pio1) than any instances of GamecubeBackend, or make sure that both use the same PIO instruction memory offset.

Each input source has a "scan speed" value which indicates roughly how long it takes for it to read inputs. Fast input sources are always read at the last possible moment (at least on Pico), resulting in very low latency. Conversely, slow input sources are typically read quite long before they are needed, as they are too slow to be read in response to poll. Because of this, it is more ideal to be constantly reading those inputs on a separate core. This is not possible on AVR MCUs as they are all single core, but it is possible (and easy) on the Pico/RP2040. The bottom of the default Pico config `config/pico/config.cpp` illustrates this by using core1 to read Nunchuk inputs while core0 handles everything else. See [the next section](#using-the-picos-second-core) for more information about using core1.


In each config's `setup()` function, we build up an array of input sources, and then pass it into a communication backend. The communication backend decides when to read which input sources, because inputs need to be read at different points in time for different backends. We also build an array of communication backends, allowing more than one backend to be used at once. For example, in most configs, the B0XX input viewer backend is used as a secondary backend whenever the DInput backend is used. In each iteration, the main loop tells each of the backends to send their respective reports. In future, there could be more backends for things like writing information to an OLED display.

### Using the Pico's second core

In each config, there are the functions `setup()` and `loop()`, where `setup()` runs first, and then `loop()` runs repeatedly until the device is powered off.

On Pico/RP2040, the `setup()` and `loop()` functions execute on core0, and you can add the functions `setup1()` and `loop1()` in order to run tasks on core1.

For example, to read GameCube controller inputs on core1:
```
GamecubeControllerInput *gcc = nullptr;

void setup1() {
    while (backends == nullptr) {
        tight_loop_contents();
    }

    gcc = new GamecubeControllerInput(gcc_pin, 2500, pio1);
}

void loop1() {
    if (backends != nullptr) {
        gcc->UpdateInputs(backends[0]->GetInputs());
    }
}
```

The `while` loop makes sure we wait until `setup()` on core0 has finished setting up the communication backends. We then create a GameCube controller input source with a polling rate of 2500Hz. We also run it on `pio1` as an easy way to avoid interfering with any GameCube/N64 backends, which use `pio0` unless otherwise specified. In `loop1()` we make the assumption that the primary backend is the first element of the `backends` array (which is configured in the same file anyway, so we aren't truly assuming anything we don't know) and directly scan the GameCube controller inputs into the backend's input state.

As a slightly crazier hypothetical example, one could even power all the controls for a two person arcade cabinet using a single Pico by creating two switch matrix input sources using say 10 pins each, and two GameCube backends, both on separate cores. The possibilities are endless.

## Troubleshooting

### Controller not working with console or GameCube adapter

If you are using an official adapter with an Arduino-based controller you will likely have to hold A on plugin which disables the polling latency optimisation by passing in a polling rate of 0 to the GamecubeBackend constructor.

If you are using an Arduino-based controller without a boost circuit, you will need 5V power so for Mayflash adapter you need both USB cables plugged in, and on console the rumble line needs to be intact. Pico works natively with 3.3V power so this isn't an issue.

## Contributing

I welcome contributions and if you make an input mode that you want to share,
feel free to make a pull request. Please install the clang-format plugin for
VS Code and use it to format any code you want added.

### Versioning

We use [SemVer](http://semver.org/) for versioning. For the versions available,
see the [tags on this repository](https://github.com/JonnyHaystack/HayBox/tags).

## Built With

* [https://github.com/MHeironimus/ArduinoJoystickLibrary]() - Used for DInput support for AVR
* [https://github.com/NicoHood/Nintendo]() - Used for GameCube and Nintendo 64 support for AVR
* [https://github.com/JonnyHaystack/ArduinoKeyboard]() - Used for keyboard modes on AVR
* [https://github.com/JonnyHaystack/arduino-nunchuk]() - Used for Nunchuk support
* [https://github.com/JonnyHaystack/joybus-pio]() - Used for GameCube and Nintendo 64 support for RP2040
* [https://github.com/earlephilhower/arduino-pico]() - Used for Arduino framework/PlatformIO support for Pico

## Contributors

* **Jonathan Haylett** - *Creator* - [@JonnyHaystack](https://github.com/JonnyHaystack)
* **Crane** - *Creator of the DIYB0XX firmware, which served as a great reference/starting point* - [@Crane1195](https://github.com/Crane1195)

See also the list of [contributors](https://github.com/JonnyHaystack/HayBox/contributors) who participated in this project.

### Acknowledgments

* The B0XX team, for designing and creating an incredible controller
* [@Crane1195](https://github.com/Crane1195) - for his DIYB0XX and GCCPCB projects, and for hours of answering my questions when I was first writing this
* [@MHeironimus](https://github.com/MHeironimus) - for the Arduino Joystick library
* [@NicoHood](https://github.com/NicoHood) - for the Nintendo library
* [@GabrielBianconi](https://github.com/GabrielBianconi) - for the Arduino Nunchuk library
* [@earlephilhower](https://github.com/earlephilhower) - for the arduino-pico core
* [@maxgerhardt](https://github.com/maxgerhardt) - for adding PlatformIO support for arduino-pico
* The Arduino project and the Raspberry Pi Foundation - for all their open-source hardware and software

## License

This project is licensed under the GNU GPL Version 3 - see the [LICENSE](LICENSE) file for details
