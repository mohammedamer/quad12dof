# Quad12DoF

## Arduino CLI workflow

Install the Wi-Fi libraries maintained by
[ESP32Async](https://github.com/ESP32Async/ESPAsyncWebServer)
(the Library Manager names contain spaces):

```sh
arduino-cli lib update-index
arduino-cli lib install "Async TCP@3.5.0" "ESP Async WebServer@3.12.1"
```

Build the quadruped sketch:

```sh
scripts/build.sh
```

Upload to the connected Adafruit Feather ESP32-S3:

```sh
scripts/upload.sh
```

Add `--monitor` (or `-m`) to open the serial monitor after a successful upload:

```sh
scripts/upload.sh --monitor
scripts/flash.sh --monitor # Build, upload, then monitor
```

The monitor uses the upload port and `ARDUINO_BAUD` (115200 by default).

Open the serial monitor at 115200 baud:

```sh
scripts/monitor.sh
```

The scripts auto-detect the board port from the default FQBN:

```sh
esp32:esp32:adafruit_feather_esp32s3
```

Override defaults when needed:

```sh
ARDUINO_PORT=/dev/cu.usbmodem101 scripts/upload.sh
ARDUINO_BAUD=9600 scripts/monitor.sh
ARDUINO_FQBN=esp32:esp32:adafruit_feather_esp32s3 scripts/build.sh
```

## Wi-Fi controller

After uploading, connect to Wi-Fi **Quadruped** with password **robot1234**.
Open **http://192.168.4.1/** (the address is also printed in the serial monitor).

Tap Forward/Backward or Left/Right to toggle a direction. Opposites replace each
other; the two axes can be combined. The center circle clears both axes. Selected
buttons are highlighted. The page reconnects automatically and clears selections
on disconnect, reconnect, or when hidden. Connecting or disconnecting any browser
queues `STOP` and clears the other connected controllers too.

`handleRobotCommand()` in `src/quadruped/quadruped.ino` stores the requested
`currentCommand`. Incoming strings are validated by `parseCommand()` and converted
to `RobotCommand`; unknown strings are ignored. Logging occurs once per received
state. States are `STOP`, `FORWARD`, `BACKWARD`, `LEFT`, `RIGHT`, `FORWARD_LEFT`,
`FORWARD_RIGHT`, `BACKWARD_LEFT`, and `BACKWARD_RIGHT`. Each replaces the entire
previous selection; the queue keeps only the latest pending state.

`updateGait()` advances four phases using `millis()`, with 100 ms between phases.
Movement changes take effect at the next cycle boundary. Stop cancels the cycle
and calls `setNeutral()` when received by the loop, without waiting for a phase
timer. Restarting movement begins at phase zero. Physical servo motion still takes
time; the loop remains available between phase updates.

The page is embedded in `src/quadruped/controller_page.h`; no internet connection
or separate file upload is required.
