"""
IoT Fire Detection and Alert System - Telegram gateway (runs on the laptop)

Reads the CSV line sent by the Arduino Uno over USB serial:
    temp_C,humidity_%,smoke,flame,status        e.g.  31.0,62,210,0,SAFE
and sends Telegram notifications to every subscriber:
    * when the status changes (SAFE -> WARNING -> FIRE -> back to SAFE)
    * repeatedly every REPEAT_FIRE_SECONDS while the status stays FIRE
    * replies to /status with the latest readings
Anyone who opens the bot and sends /start is subscribed automatically
(/stop unsubscribes). Subscribers are remembered in subscribers.json.
All readings are also saved with a timestamp in fire_log.csv.

"""
import csv
import json
import os
import time
from datetime import datetime

import requests
import serial

from config import BOT_TOKEN, CHAT_ID, SERIAL_PORT   # see config_example.py

BAUD_RATE = 9600
REPEAT_FIRE_SECONDS = 30
LOG_FILE = "fire_log.csv"
SUBS_FILE = "subscribers.json"
API = f"https://api.telegram.org/bot{BOT_TOKEN}"

last = None            # latest reading: dict
last_status = None
last_fire_alert = 0.0
update_offset = 0
last_poll = 0.0


def load_subscribers():
    subs = {str(CHAT_ID)}                       # the owner is always subscribed
    if os.path.exists(SUBS_FILE):
        try:
            with open(SUBS_FILE) as f:
                subs.update(str(x) for x in json.load(f))
        except (ValueError, OSError):
            pass
    return subs


def save_subscribers():
    with open(SUBS_FILE, "w") as f:
        json.dump(sorted(subscribers), f)


subscribers = load_subscribers()


def send(text, chat_id=None):
    """Send to one chat, or to every subscriber when chat_id is None."""
    targets = [chat_id] if chat_id is not None else sorted(subscribers)
    for cid in targets:
        try:
            requests.post(f"{API}/sendMessage",
                          data={"chat_id": cid, "text": text}, timeout=10)
        except requests.RequestException as err:
            print("Telegram send failed for", cid, ":", err)


def describe(r):
    return (f"Temp: {r['temp']} C\nHumidity: {r['hum']} %\n"
            f"Smoke level: {r['smoke']}\nFlame: {'YES' if r['flame'] else 'NO'}\n"
            f"Time: {datetime.now():%d-%m-%Y %H:%M:%S}")


def parse(line):
    """Return a dict for a valid CSV line, otherwise None (header, noise)."""
    parts = line.strip().split(",")
    if len(parts) != 5 or parts[4] not in ("SAFE", "WARNING", "FIRE"):
        return None
    try:
        return {"temp": float(parts[0]), "hum": int(float(parts[1])),
                "smoke": int(parts[2]), "flame": int(parts[3]), "status": parts[4]}
    except ValueError:
        return None


def log(r):
    new_file = not os.path.exists(LOG_FILE)
    with open(LOG_FILE, "a", newline="") as f:
        w = csv.writer(f)
        if new_file:
            w.writerow(["timestamp", "temp_C", "humidity_%", "smoke", "flame", "status"])
        w.writerow([datetime.now().isoformat(timespec="seconds"),
                    r["temp"], r["hum"], r["smoke"], r["flame"], r["status"]])


def handle_reading(r):
    global last, last_status, last_fire_alert
    last = r
    log(r)
    now = time.time()
    if r["status"] != last_status:
        if r["status"] == "FIRE":
            send("FIRE DETECTED!\n" + describe(r) + "\nEvacuate and call the fire department.")
            last_fire_alert = now
        elif r["status"] == "WARNING":
            send("WARNING: abnormal smoke / temperature\n" + describe(r))
        elif last_status is not None:
            send("Back to SAFE. Readings are normal.\n" + describe(r))
        last_status = r["status"]
    elif r["status"] == "FIRE" and now - last_fire_alert >= REPEAT_FIRE_SECONDS:
        send("STILL ON FIRE!\n" + describe(r))
        last_fire_alert = now


def poll_commands():
    """Handle /start, /stop, /status and /help sent to the bot."""
    global update_offset
    try:
        data = requests.get(f"{API}/getUpdates",
                            params={"offset": update_offset, "timeout": 0},
                            timeout=10).json()
    except (requests.RequestException, ValueError):
        return
    for u in data.get("result", []):
        update_offset = u["update_id"] + 1
        msg = u.get("message", {})
        cid = str(msg.get("chat", {}).get("id", ""))
        cmd = msg.get("text", "").strip().lower().split("@")[0]
        if not cid:
            continue
        name = msg.get("from", {}).get("first_name", "friend")
        if cmd == "/start":
            if cid not in subscribers:
                subscribers.add(cid)
                save_subscribers()
                print("New subscriber:", name, cid)
            send(f"Hi {name}! You will now receive fire alerts.\n"
                 "/status - latest readings\n/stop - unsubscribe", cid)
        elif cid not in subscribers:
            send("Send /start to subscribe to fire alerts.", cid)
        elif cmd == "/stop":
            if cid == str(CHAT_ID):                    # owner always stays subscribed
                send("The owner cannot unsubscribe.", cid)
            else:
                subscribers.discard(cid)
                save_subscribers()
                send("Unsubscribed. Send /start to subscribe again.", cid)
        elif cmd == "/status":
            send(("Status: " + last["status"] + "\n" + describe(last))
                 if last else "No reading received yet.", cid)
        elif cmd == "/help":
            send("/status - latest readings\n/stop - unsubscribe", cid)


def main():
    global last_poll
    print(f"Opening {SERIAL_PORT} ...")
    ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
    send("Fire detection gateway started. Send /status for readings.", str(CHAT_ID))
    while True:
        try:
            line = ser.readline().decode("utf-8", errors="ignore")
        except serial.SerialException:
            print("Serial connection lost. Check the USB cable.")
            send("Gateway lost connection to the Arduino!")
            break
        if line:
            print(line.strip())
            r = parse(line)
            if r:
                handle_reading(r)
        if time.time() - last_poll >= 3:
            last_poll = time.time()
            poll_commands()


if __name__ == "__main__":
    main()
