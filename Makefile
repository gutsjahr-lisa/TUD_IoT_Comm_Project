# name of your application
APPLICATION = iot-hw

# If no BOARD is found in the environment, use this default:
BOARD ?= adafruit-feather-nrf52840-sense

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/RIOT

# Comment this out to disable code in RIOT that does safety checking
# which is not needed in a production environment but helps in the
# development process:
DEVELHELP ?= 1

# Change this to 0 show compiler invocation lines by default:
QUIET ?= 1

# 802.15.4 radio driver and dependencies
USEMODULE += nrf802154
USEMODULE += netdev_ieee802154_submac
USEMODULE += event_thread
USEMODULE += ztimer
USEMODULE += ztimer_msec

# Build as TX node with: make TX_NODE=1 NODE_ID=A
TX_NODE ?= 0
NODE_ID ?= A
CFLAGS += -DTX_NODE=$(TX_NODE)
CFLAGS += -DNODE_ID='"$(NODE_ID)"'
CFLAGS += -DEVENT_THREAD_STACKSIZE_DEFAULT=2048

init:
	git submodule update --init --recursive

include $(RIOTBASE)/Makefile.include
