import tkinter as tk
import csv
import subprocess
import serial
import os


CSV_PATH = "/home/fantine/pantone_project/pantone_rgb_mapping.csv"
SERIAL_PORT = "/dev/ttyACM0"
BAUD_RATE = 115200
UPDATE_MS = 100


def get_ip():
    try:
        result = subprocess.check_output(["hostname", "-I"]).decode().strip()
        return result.split()[0] if result else "No IP"
    except:
        return "No IP"


def rgb_to_hex(r, g, b):
    return f"#{r:02x}{g:02x}{b:02x}"


def clamp(v):
    return max(0, min(255, int(v)))


def load_pantone_data(path):
    data = []

    if not os.path.exists(path):
        return data, f"CSV not found: {path}"

    try:
        with open(path, newline="", encoding="utf-8-sig") as f:
            sample = f.read(2048)
            f.seek(0)

            delimiter = ";"

            if sample.count(",") > sample.count(";"):
                delimiter = ","

            reader = csv.DictReader(f, delimiter=delimiter)

            if reader.fieldnames is None:
                return data, "CSV header not found"

            fields = [x.strip() for x in reader.fieldnames]
            required = ["pantone_name", "rgb_r", "rgb_g", "rgb_b"]

            if not all(col in fields for col in required):
                return data, f"Wrong CSV columns: {fields}"

            for row in reader:
                try:
                    name = row["pantone_name"].strip()
                    r = clamp(row["rgb_r"])
                    g = clamp(row["rgb_g"])
                    b = clamp(row["rgb_b"])

                    data.append((name, r, g, b))
                except:
                    pass

        if not data:
            return data, "CSV loaded but no valid rows found"

        return data, f"{len(data)} Pantone rows loaded"

    except Exception as e:
        return data, f"CSV read error: {e}"


def find_nearest_pantone(r, g, b, data):
    best = None
    best_dist = float("inf")

    for name, pr, pg, pb in data:
        dist = (r - pr) ** 2 + (g - pg) ** 2 + (b - pb) ** 2

        if dist < best_dist:
            best_dist = dist
            best = (name, pr, pg, pb)

    return best


def parse_rgb_line(line):
    line = line.strip().replace(",", " ")
    parts = line.split()

    if len(parts) < 3:
        return None

    try:
        r = clamp(parts[0])
        g = clamp(parts[1])
        b = clamp(parts[2])

        return r, g, b

    except:
        return None


pantone_data, csv_status = load_pantone_data(CSV_PATH)

ser = None

try:
    ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=0.1)
except:
    ser = None


root = tk.Tk()

root.title("RGB Pantone Matcher")
root.attributes("-fullscreen", True)
root.configure(bg="white")


def exit_app():
    global ser

    try:
        if ser:
            ser.close()
    except:
        pass

    root.destroy()


title_label = tk.Label(
    root,
    text="RGB Pantone Matcher",
    font=("Arial", 20, "bold"),
    bg="white"
)
title_label.pack(pady=(20, 10))


ip_label = tk.Label(
    root,
    text=f"IP: {get_ip()}",
    font=("Arial", 12),
    bg="white"
)
ip_label.pack(pady=(0, 10))


csv_label = tk.Label(
    root,
    text=csv_status,
    font=("Arial", 11),
    bg="white",
    fg="blue"
)
csv_label.pack(pady=(0, 10))


main_frame = tk.Frame(root, bg="white")
main_frame.pack(
    expand=True,
    fill="both",
    padx=30,
    pady=20
)


left_frame = tk.Frame(main_frame, bg="white")
left_frame.pack(
    side="left",
    expand=True,
    fill="both",
    padx=20
)


right_frame = tk.Frame(main_frame, bg="white")
right_frame.pack(
    side="right",
    expand=True,
    fill="both",
    padx=20
)


sensor_title = tk.Label(
    left_frame,
    text="Sensor Color",
    font=("Arial", 16, "bold"),
    bg="white"
)
sensor_title.pack(pady=(0, 10))


sensor_box = tk.Label(
    left_frame,
    bg="#000000",
    width=18,
    height=9
)
sensor_box.pack(pady=10)


sensor_rgb = tk.Label(
    left_frame,
    text="RGB: -",
    font=("Arial", 14),
    bg="white"
)
sensor_rgb.pack()


pantone_title = tk.Label(
    right_frame,
    text="Nearest Pantone",
    font=("Arial", 16, "bold"),
    bg="white"
)
pantone_title.pack(pady=(0, 10))


pantone_box = tk.Label(
    right_frame,
    bg="#000000",
    width=18,
    height=9
)
pantone_box.pack(pady=10)


pantone_name = tk.Label(
    right_frame,
    text="Pantone: -",
    font=("Arial", 14, "bold"),
    bg="white",
    wraplength=320,
    justify="center"
)
pantone_name.pack()


pantone_rgb = tk.Label(
    right_frame,
    text="RGB: -",
    font=("Arial", 14),
    bg="white"
)
pantone_rgb.pack()


status_label = tk.Label(
    root,
    text="Waiting for serial data...",
    font=("Arial", 12),
    bg="white"
)
status_label.pack(pady=10)


exit_button = tk.Button(
    root,
    text="Exit",
    command=exit_app,
    bg="red",
    fg="white"
)
exit_button.place(
    relx=1.0,
    x=-20,
    y=20,
    anchor="ne"
)


root.bind("<Escape>", lambda e: exit_app())


def update_ip():
    ip_label.config(text=f"IP: {get_ip()}")
    root.after(5000, update_ip)


def update_gui():
    global ser

    if ser is None:
        try:
            ser = serial.Serial(
                SERIAL_PORT,
                BAUD_RATE,
                timeout=0.1
            )

            status_label.config(text="Serial connected")

        except:
            status_label.config(
                text=f"Serial not connected: {SERIAL_PORT}"
            )

            root.after(1000, update_gui)
            return

    try:
        if ser.in_waiting:
            line = ser.readline().decode(
                "utf-8",
                errors="ignore"
            )

            rgb = parse_rgb_line(line)

            if rgb:
                r, g, b = rgb

                sensor_box.config(
                    bg=rgb_to_hex(r, g, b)
                )

                sensor_rgb.config(
                    text=f"RGB: {r}, {g}, {b}"
                )

                nearest = find_nearest_pantone(
                    r,
                    g,
                    b,
                    pantone_data
                )

                if nearest:
                    name, pr, pg, pb = nearest

                    pantone_box.config(
                        bg=rgb_to_hex(pr, pg, pb)
                    )

                    pantone_name.config(
                        text=f"Pantone: {name}"
                    )

                    pantone_rgb.config(
                        text=f"RGB: {pr}, {pg}, {pb}"
                    )

                    status_label.config(
                        text="Pantone matched"
                    )

                else:
                    pantone_name.config(
                        text="Pantone: not found"
                    )

                    pantone_rgb.config(
                        text="RGB: -"
                    )

                    status_label.config(
                        text="Pantone data is empty or invalid"
                    )

    except:
        try:
            ser.close()
        except:
            pass

        ser = None

        status_label.config(
            text="Serial read error, reconnecting..."
        )

    root.after(UPDATE_MS, update_gui)


update_ip()
update_gui()

root.mainloop()

