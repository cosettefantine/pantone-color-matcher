# Raspberry Pi Kiosk Mode

This document describes how the Raspberry Pi is configured to operate as a dedicated user-interface device for the **Pantone Color Matcher** project.

The purpose of the kiosk-style configuration is to allow the system to start the Pantone matching application automatically after boot and present the graphical interface in fullscreen mode.

This removes the need to manually open a terminal and launch the application during normal use.

---

## Purpose

The Raspberry Pi is used as the high-level processing and graphical interface unit of the system.

During normal operation, the intended startup sequence is:

1. The Raspberry Pi is powered on.
2. Raspberry Pi OS starts from the microSD card.
3. The Pantone Python application is launched automatically.
4. The Tkinter interface starts in fullscreen mode.
5. The Pantone CSV dataset is loaded.
6. The application attempts to connect to the Arduino through serial communication.
7. RGB measurements are received continuously.
8. The nearest Pantone shade is calculated.
9. The result is displayed on the touchscreen.

This configuration allows the prototype to behave more like a dedicated color matching device than a general-purpose computer.

---

## Operating System

The project uses:

```text
Raspberry Pi OS Lite
```

Raspberry Pi OS Lite is installed on the Raspberry Pi's microSD card.

The project does not require a full conventional desktop environment for its primary functionality. The Raspberry Pi mainly runs the software required for:

- Python execution
- Tkinter graphical interface
- Serial communication
- Pantone dataset processing
- Pantone color matching

---

## Project Deployment

The project files are transferred to the Raspberry Pi through an **SSH connection**.

The application and Pantone dataset are stored locally on the Raspberry Pi.

The project uses a directory similar to:

```text
/home/fantine/pantone_project/
```

The main files include:

```text
pantone_project/

pantone_gui.py
pantone_rgb_mapping.csv
```

The Python application expects the Pantone dataset at:

```python
CSV_PATH="/home/fantine/pantone_project/pantone_rgb_mapping.csv"
```

---

## Automatic Application Startup

The Raspberry Pi is configured so that the Pantone application can be launched automatically during system startup.

The intended kiosk behavior can be implemented using one of the following methods:
- Startup scripts
- `systemd` services
- Lightweight desktop-session autostart configuration

The purpose of the startup configuration is to execute the Python application without requiring the user to manually start it after each reboot.

The application command is conceptually:

```bash
python3 pantone-gui.py
```

The exact startup mechanism depends on the final Raspberry Pi software configuration used for the prototype.

---

## Fullscreen Interface

The Python application uses Tkinter for the graphical interface.

The main window is created with:

```python
root=tk.Tk()

root.title("RGB Pantone Matcher")

root.attributes("-fullscreen",True)

root.configure(bg="white")
```

Fullscreen mode is enabled with:

```python
root.attributes("-fullscreen",True)
```

This causes the Pantone application to occupy the available display area.

The user therefore interacts primarily with the Pantone matching interface instead of a conventional application window.

---

## Graphical Interface

After startup, the application displays information including:

- Application title
- Raspberry Pi IP address
- CSV loading status
- Measured sensor color
- Measured RGB values
- Nearest Pantone color
- Pantone shade name
- Pantone RGB values
- Serial communication status
- Exit button

The interface is designed to present the information required for normal color measurement without requiring terminal interaction.

---

## Raspberry Pi IP Address

The application displays the Raspberry Pi's current IP address.

The IP address is obtained using:

```python
def get_ip():
    try:
        result=subprocess.check_output(
            ["hostname","-I"]
        ).decode().strip()

        return result.split()[0] if result else "No IP"

    except:
        return "No IP"
```

The value is displayed in the Tkinter interface:

```python
ip_label=tk.Label(
    root,
    text=f"IP: {get_ip()}",
    font=("Arial",12),
    bg="white"
)
```

Displaying the IP address is useful during development and maintenance because the Raspberry Pi can be accessed remotely through SSH.

---

## Automatic IP Update

The displayed IP address is periodically refreshed.

```python
def update_ip():
    ip_label.config(
        text=f"IP: {get_ip()}"
    )

    root.after(
        5000,
        update_ip
    )
```

The update interval is:

```text
5000 ms
```

This corresponds to approximately:

```text
5 seconds
```

The periodic update allows the interface to show a new address if the Raspberry Pi's network configuration changes.

---

## Arduino Serial Connection

The Raspberry Pi receives processed RGB measurements from the Arduino Mega through USB serial communication.

The application uses:

```python
SERIAL_PORT="/dev/ttyACM0"
BAUD_RATE=115200
```

The configured serial port is:

```text
/dev/ttyACM0
```

The communication rate is:

```text
115200 baud
```

During startup, the application attempts to open the serial connection:

```python
ser=None

try:
    ser=serial.Serial(
        SERIAL_PORT,
        BAUD_RATE,
        timeout=0.1
    )

except:
    ser=None
```

If the Arduino is unavailable, the application can continue running instead of terminating immediately.

---

## Automatic Serial Reconnection

The GUI periodically checks whether a serial connection exists.

If the serial connection is unavailable, the program attempts to reconnect:

```python
if ser is None:

    try:
        ser=serial.Serial(
            SERIAL_PORT,
            BAUD_RATE,
            timeout=0.1
        )

        status_label.config(
            text="Serial connected"
        )

    except:
        status_label.config(
            text=f"Serial not connected: {SERIAL_PORT}"
        )

        root.after(
            1000,
            update_gui
        )

        return
```

This behavior improves reliability during startup because the Arduino may not always become available at exactly the same time as the Raspberry Pi application.

---

## Serial Error Recovery

If an error occurs while reading serial data, the application attempts to close the existing serial connection.

```python
try:
    ser.close()

except:
    pass
```

The serial object is then reset:

```python
ser=None
```

The GUI displays:

```text
Serial read error, reconnecting...
```

A later GUI update can attempt to establish the serial connection again.

This allows the interface to remain open during temporary communication problems.

---

## GUI Update Cycle

The application uses Tkinter's `after()` method to repeatedly check for incoming measurements.

The update interval is defined as:

```python
UPDATE_MS=100
```

The update function schedules itself again with:

```python
root.after(
    UPDATE_MS,
    update_gui
)
```

This causes the application to check for new serial data approximately every:

```text
100 ms
```

Using Tkinter's scheduled callbacks allows the program to continue updating measurements without permanently blocking the graphical interface.

---

## Runtime Processing

During normal operation, the system repeatedly performs the following tasks:

1. The Arduino measures the physical object's color.
2. The Arduino performs RGB preprocessing and calibration.
3. The processed RGB values are transmitted through serial communication.
4. The Raspberry Pi reads the serial data.
5. The Python application parses the RGB values.
6. The measured color is displayed.
7. The Pantone dataset is searched.
8. The nearest Pantone color is selected.
9. The Pantone name and RGB values are displayed.
10. The process repeats when new sensor data becomes available.

The user does not need to manually trigger the color matching operation for every measurement.

---

## Pantone Dataset Loading

The application loads the Pantone RGB dataset when it starts.

The dataset path is:

```python
CSV_PATH="/home/fantine/pantone_project/pantone_rgb_mapping.csv"
```

The program checks whether the file exists before attempting to load it.

```python
if not os.path.exists(path):
    return data,f"CSV not found: {path}"
```

The expected columns are:

```text
pantone_name
rgb_r
rgb_g
rgb_b
```

The application also detects whether the CSV file uses:

```text
;
```

or:

```text
,
```

as its delimiter.

The CSV loading result is shown directly in the GUI.

---

## Pantone Matching

When a valid RGB measurement is received, the Raspberry Pi compares it with the Pantone RGB dataset.

The current application calculates the squared RGB distance using:

```python
dist=(r-pr)**2+(g-pg)**2+(b-pb)**2
```

The Pantone entry with the smallest calculated distance is selected.

The interface then displays:

- Measured RGB color
- Measured RGB values
- Matched Pantone color
- Pantone name
- Pantone RGB values

---

## Kiosk-Style User Experience

Without automatic startup, the user would need to perform several manual operations each time the Raspberry Pi starts.

For example:

1. Wait for the operating system to boot.
2. Open a terminal.
3. Navigate to the project directory.
4. Start the Python application manually.
5. Switch the application to the desired display state.

The kiosk-style configuration is intended to reduce this process to:

1. Power on the device.
2. Wait for Raspberry Pi startup.
3. Use the Pantone matching interface.

This is more appropriate for an embedded prototype intended to function as a dedicated device.

---

## Exiting the Application

Although the interface normally runs in fullscreen mode, the program includes a manual exit function.

An exit button is created using:

```python
exit_button=tk.Button(
    root,
    text="Exit",
    command=exit_app,
    bg="red",
    fg="white"
)
```

The `Escape` key can also close the application:

```python
root.bind(
    "<Escape>",
    lambda e: exit_app()
)
```

This provides a convenient way to leave fullscreen mode during testing or maintenance.

---

## Safe Application Exit

The application attempts to close the serial connection before terminating.

```python
def exit_app():

    global ser

    try:
        if ser:
            ser.close()

    except:
        pass

    root.destroy()
```

This prevents the application from intentionally leaving its current serial connection open when the GUI is closed.

---

## Startup Sequence

The complete startup procedure can be described as follows:

| Step | Process |
|---:|---|
| 1 | Raspberry Pi receives power |
| 2 | Raspberry Pi OS Lite boots |
| 3 | System services and required environment initialize |
| 4 | Pantone Python application starts automatically |
| 5 | Tkinter GUI is created |
| 6 | Fullscreen mode is enabled |
| 7 | Pantone CSV file is loaded |
| 8 | Raspberry Pi IP address is detected |
| 9 | Serial connection to Arduino is attempted |
| 10 | RGB measurements are received |
| 11 | Pantone matching is performed |
| 12 | GUI is continuously updated |

---

## Periodic Operations

Two important recurring operations are used by the application.

### GUI and Serial Update

Configured interval:

```text
100 ms
```

Implemented using:

```python
root.after(
    UPDATE_MS,
    update_gui
)
```

### IP Address Update

Configured interval:

```text
5 seconds
```

Implemented using:

```python
root.after(
    5000,
    update_ip
)
```

These scheduled callbacks allow the application to perform periodic tasks while keeping the interface responsive.

---

## Failure Handling

The application includes several basic error-handling mechanisms that are useful for kiosk-style operation.

### Pantone CSV Not Found

If the dataset cannot be found, the program generates a status similar to:

```text
CSV not found
```

The error can be displayed without immediately closing the entire GUI.

### Arduino Not Connected

If the configured serial device cannot be opened, the GUI can display:

```text
Serial not connected
```

The application remains active and attempts the connection again later.

### Serial Read Error

If an error occurs while reading serial data, the application displays:

```text
Serial read error, reconnecting...
```

The serial object is reset so that the application can retry the connection.

---

## Software Responsibilities

### Arduino Mega 2560

The Arduino is responsible for:

- Reading the TCS34725 color sensor
- Controlling the pan-tilt servos
- Reading joystick input
- Applying calibration
- Applying RGB preprocessing
- Sending processed RGB values through serial communication

### Raspberry Pi 3 Model B

The Raspberry Pi is responsible for:

- Running the Python application
- Loading the Pantone CSV dataset
- Receiving RGB values from Arduino
- Finding the nearest Pantone shade
- Displaying the graphical interface
- Showing system status
- Displaying network information

This separation allows sensor-related operations to remain on the microcontroller while the Raspberry Pi handles the graphical and higher-level processing tasks.

---

## Kiosk Configuration Summary

| Feature | Implementation |
|---|---|
| Operating system | Raspberry Pi OS Lite |
| Application language | Python |
| GUI library | Tkinter |
| Display mode | Fullscreen |
| Arduino connection | USB Serial |
| Serial port | `/dev/ttyACM0` |
| Baud rate | 115200 |
| GUI update interval | 100 ms |
| IP update interval | 5 seconds |
| Dataset | Local Pantone RGB CSV |
| Remote maintenance | SSH |
| Application startup | Automatic during system startup |
| Serial recovery | Automatic retry |
| Manual exit | Exit button and Escape key |

---

## Summary

The Raspberry Pi configuration allows the **Pantone Color Matcher** to operate as a standalone embedded interface.

The kiosk-style implementation combines:

- Raspberry Pi OS Lite
- Automatic Python application startup
- Fullscreen Tkinter GUI
- Local Pantone CSV loading
- USB serial communication with Arduino
- Automatic serial reconnection
- Periodic IP address updates
- Continuous RGB processing
- Real-time Pantone matching
- Touch-display-oriented operation

The result is a system that can be powered on and used primarily through the Pantone matching interface without requiring normal desktop interaction during everyday operation.
