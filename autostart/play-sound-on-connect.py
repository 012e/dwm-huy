#!/usr/bin/env -S uv run --script
#
# /// script
# requires-python = ">=3.12"
# dependencies = ["playsound3", "pyudev"]
# ///

import logging
import pathlib
import sys

import pyudev
from playsound3 import playsound

# Configure logging as required
logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s - %(levelname)s - %(message)s",
    stream=sys.stdout,
)
logger = logging.getLogger(__name__)

CONNECTED_SOUND_FILE_PATH = pathlib.Path(__file__).parent.resolve() / "connected.mp3"
DISCONNECTED_SOUND_FILE_PATH = (
    pathlib.Path(__file__).parent.resolve() / "disconnected.mp3"
)


def play_connected_sound():
    playsound(CONNECTED_SOUND_FILE_PATH, backend="ffplay")


def play_disconnected_sound():
    playsound(DISCONNECTED_SOUND_FILE_PATH, backend="ffplay")


def monitor_devices():
    """
    Monitors for device connect/disconnect events for both USB and Bluetooth
    subsystems using pyudev.
    """
    context = pyudev.Context()
    monitor = pyudev.Monitor.from_netlink(context)

    # Filter on the kernel side for both 'usb' and 'bluetooth' subsystems.
    # This reduces the total number of events received from udev.
    monitor.filter_by(subsystem="usb")
    monitor.filter_by(subsystem="bluetooth")

    logger.info("Listening for USB and Bluetooth device connect/disconnect events...")

    # Synchronously listen for events
    for device in iter(monitor.poll, None):
        action = device.action
        subsystem = device.subsystem

        # --- Python Filtering and Processing ---

        if subsystem == "usb":
            # 1. Focus on the main USB device event ('usb_device')
            # This is a good proxy for the physical plug/unplug event.
            if device.get("DEVTYPE") == "usb_device":
                vendor_id = device.get("ID_VENDOR_ID", "N/A")
                product_id = device.get("ID_MODEL_ID", "N/A")

                device_info = (
                    f"{device.device_path} (VID: {vendor_id}, PID: {product_id})"
                )

                if action == "add":
                    logger.info(f"✅ USB DEVICE CONNECTED: {device_info}")
                    play_connected_sound()

                elif action == "remove":
                    logger.info(f"❌ USB DEVICE DISCONNECTED: {device_info}")
                    play_disconnected_sound()

        elif subsystem == "bluetooth":
            # 2. Focus on Bluetooth events related to device connection/disconnection.
            # The 'change' action on a 'bluetooth' subsystem device often signals state change.
            # The 'DEVTYPE' filter depends on the kernel/system, but we primarily look for
            # actions that indicate a device appearing/disappearing.

            # The 'ID_PATH' or 'ID_NAME' can be used to identify the device.
            device_name = device.get(
                "ID_NAME", device.get("ID_BLUETOOTH_NAME", "Unknown Bluetooth Device")
            )

            # Look for 'add' and 'remove' actions on a bluetooth device node.
            # Note: Bluetooth event handling can be more complex/varied than USB;
            # this catches the most common device enumeration events.
            if action == "add":
                logger.info(f"➕ BLUETOOTH DEVICE DETECTED/READY: {device_name}")
                play_connected_sound()
            elif action == "remove":
                logger.info(f"➖ BLUETOOTH DEVICE REMOVED/LOST: {device_name}")
                play_disconnected_sound()

            # Additional filtering to catch connection status changes is often needed,
            # but is highly dependent on specific udev rules.
            # This change event can often signal a connection status change:
            elif action == "change":
                # Look for a device property that indicates a status change, e.g.,
                # a change to the power state, or specific connection properties
                # exposed by the kernel. For now, we'll log general changes.
                logger.debug(
                    f"ℹ️ BLUETOOTH DEVICE CHANGED: {device_name} (Action: {action})"
                )


if __name__ == "__main__":
    try:
        monitor_devices()
    except KeyboardInterrupt:
        logger.info("Monitoring stopped by user.")
    except Exception as e:
        logger.error(f"An unexpected error occurred: {e}")
