# IoT RSSI Logger — IEEE 802.15.4

Identifies deployment environments and IoT sensor nodes using link quality fluctuations (RSSI/LQI) over IEEE 802.15.4 radio. Built on RIOT OS for the Adafruit Feather nRF52840 Sense board.

## How it works

One node is flashed as **TX**: it broadcasts ~10 packets/second containing its compiled-in node identity (e.g. `A`).

All other nodes are flashed as **RX**: they receive packets and log `node_id, timestamp, RSSI, LQI` as CSV to serial (USB CDC).

Data is collected across 5 deployment environments (bridge, garden, forest, river, lake) and later used to train CNN/ResNet models to classify environments and identify individual nodes.

## Requirements

- Adafruit Feather nRF52840 Sense boards (3–5 per deployment)
- ARM cross-compiler: `arm-none-eabi-gcc`
- RIOT OS (included as a git submodule in `RIOT/`)

## First-time setup

Clone and initialise the RIOT submodule:

```bash
git clone <this-repo>
cd iot_project
make init
```

## Build

### RX node (default)

```bash
make
```

### TX node

Each TX node must be given a unique identity letter compiled into the binary:

```bash
make TX_NODE=1 NODE_ID=A
```

### Build options

| Variable  | Default | Description                        |
|-----------|---------|------------------------------------|
| `TX_NODE` | `0`     | Set to `1` to build as transmitter |
| `NODE_ID` | `A`     | Single-letter identity sent in every packet (TX only) |
| `BOARD`   | `adafruit-feather-nrf52840-sense` | Target board |

## Flash

```bash
make flash                        # flash RX firmware
make TX_NODE=1 NODE_ID=A flash    # flash TX firmware with identity A
make TX_NODE=1 NODE_ID=B flash    # flash TX firmware with identity B
```

Connect each board via USB before flashing. After flashing, press the reset button while the serial terminal is open to see startup output.

## Collect data

Open a serial terminal on an RX node:

```bash
make term
```

Save output to a file (live view + file simultaneously):

```bash
make term | tee data_bridge_nodeA.csv
```

The output format is CSV:

```
node_id,timestamp_ms,rssi_dBm,lqi
A,1234,-67,200
A,1334,-68,198
```

Run each TX node for **30 minutes** per deployment environment at maximum transmission distance.

## Cleaning the data file

`make term` output contains pyterm timestamps, `#` prefixes, and build/flash noise. Use the provided script to strip everything except the CSV rows:

```bash
./clean_csv.sh data_bridge.csv                  # produces data_bridge_clean.csv
./clean_csv.sh data_bridge.csv bridge_clean.csv  # custom output name
```

The script keeps only the header line and valid data rows, discarding all pyterm metadata. Always run it before any analysis or ML preprocessing.

## Radio configuration

| Parameter   | Value    |
|-------------|----------|
| Standard    | IEEE 802.15.4 |
| Channel     | 26 (2480 MHz) |
| PAN ID      | `0xABCD` |
| TX power    | 8 dBm (max for nRF52840) |
| Rate        | 10 packets/second |

## Data preprocessing (for ML)

Before training, preprocess the raw RSSI time series:

1. **Differentiate** to focus on change rather than absolute level:
   `y[i] = x[i+1] - x[i]`

2. **Normalise** to [0, 1]:
   `z[i] = (y[i] - y_min) / (y_max - y_min)`

3. **Segment** into overlapping frames:
   - Frame size: 500 or 1000 samples
   - Overlap: 40% or 50%

## Project structure

```
iot_project/
├── main.c          # Firmware (TX + RX roles, single source)
├── Makefile        # Build configuration
├── clean_csv.sh    # Strips pyterm noise from captured data files
├── RIOT/           # RIOT OS (git submodule)
└── README.md
```
