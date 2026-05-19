# Quad12DoF

## Arduino CLI workflow

Build the quadruped sketch:

```sh
scripts/build.sh
```

Upload to the connected Adafruit Feather ESP32-S3:

```sh
scripts/upload.sh
```

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
