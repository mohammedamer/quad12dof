#!/usr/bin/env bash

set -euo pipefail

source "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/arduino-env.sh"

require_arduino_cli

PORT="$(arduino_port)"

arduino-cli monitor \
  --port "${PORT}" \
  --config "baudrate=${ARDUINO_BAUD}"
