#!/usr/bin/env bash

set -euo pipefail

source "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/arduino-env.sh"

require_arduino_cli

arduino-cli compile \
  --fqbn "${ARDUINO_FQBN}" \
  "${SKETCH_DIR}"

