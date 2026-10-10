#!/usr/bin/env python3
"""Black metal drum pattern generator -> General MIDI .mid files (stdlib only).

Patterns are step grids. Each lane is a string, one character per step:
    .        rest
    x        normal hit
    X        accent
    g        ghost note
    1-9      explicit velocity level (1 = soft ... 9 = max), handy for crescendos
Spaces and '|' are ignored, so grids can be visually grouped by beat.

`spb` = steps per beat (4 = 16ths, 8 = 32nds, 3 = 8th triplets, 6 = sextuplets).
Grid length / spb = length in beats; grooves are 4/4, fills are one beat long.

Usage:  python3 generate.py            # writes ./midi/grooves and ./midi/fills
"""

import os
import random
import struct
import zlib

PPQ = 480
CHANNEL = 9  # MIDI channel 10 = drums

# General MIDI drum map
GM = {
    "kick": 36, "snare": 38, "rim": 37,
    "hh": 42, "hh_pedal": 44, "hh_open": 46,
    "crash": 49, "crash2": 57, "china": 52, "splash": 55,
    "ride": 51, "bell": 53,
    "tom1": 50, "tom2": 48, "tom3": 47, "floor": 43, "floor2": 41,
}

VEL = {"x": 100, "X": 120, "g": 45}
VEL.update({str(i): 30 + (i - 1) * 12 for i in range(1, 10)})  # 1=30 ... 9=126

HUMANIZE = 6        # +/- random velocity spread
BEAT_ACCENT = 5     # extra velocity on hits that land on a beat


# ---------------------------------------------------------------- patterns

GROOVES = {
    # Traditional blast: kick + cymbal together, snare in between (alternating 16ths).
    "blast_traditional_ride": dict(bpm=200, spb=4, lanes={
        "crash": "x... .... .... .... | .... .... .... ....",
        "ride":  "..x. x.x. x.x. x.x. | x.x. x.x. x.x. x.x.",
        "kick":  "x.x. x.x. x.x. x.x. | x.x. x.x. x.x. x.x.",
        "snare": ".x.x .x.x .x.x .x.x | .x.x .x.x .x.x .x.x",
    }),
    "blast_traditional_crash": dict(bpm=200, spb=4, lanes={
        "crash": "X.x. x.x. x.x. x.x. | X.x. x.x. x.x. x.x.",
        "kick":  "x.x. x.x. x.x. x.x. | x.x. x.x. x.x. x.x.",
        "snare": ".x.x .x.x .x.x .x.x | .x.x .x.x .x.x .x.x",
    }),
    "blast_traditional_hihat": dict(bpm=210, spb=4, lanes={
        "crash": "x... .... .... .... | .... .... .... ....",
        "hh":    "..x. x.x. x.x. x.x. | x.x. x.x. x.x. x.x.",
        "kick":  "x.x. x.x. x.x. x.x. | x.x. x.x. x.x. x.x.",
        "snare": ".x.x .x.x .x.x .x.x | .x.x .x.x .x.x .x.x",
    }),
    # Hammer / unison blast: kick, snare and cymbal all on the same 8ths.
    "blast_hammer_crash": dict(bpm=180, spb=4, lanes={
        "crash": "X.x. x.x. x.x. x.x. | X.x. x.x. x.x. x.x.",
        "kick":  "x.x. x.x. x.x. x.x. | x.x. x.x. x.x. x.x.",
        "snare": "x.x. x.x. x.x. x.x. | x.x. x.x. x.x. x.x.",
    }),
    "blast_hammer_china": dict(bpm=180, spb=4, lanes={
        "crash": "X... .... .... .... | .... .... .... ....",
        "china": "..x. x.x. x.x. x.x. | x.x. x.x. x.x. x.x.",
        "kick":  "x.x. x.x. x.x. x.x. | x.x. x.x. x.x. x.x.",
        "snare": "x.x. x.x. x.x. x.x. | x.x. x.x. x.x. x.x.",
    }),
    # Bomb blast: constant 16th double kick under 8th-note snare.
    "blast_bomb": dict(bpm=190, spb=4, lanes={
        "crash": "X... x... x... x... | X... x... x... x...",
        "kick":  "xxxx xxxx xxxx xxxx | xxxx xxxx xxxx xxxx",
        "snare": "x.x. x.x. x.x. x.x. | x.x. x.x. x.x. x.x.",
    }),
    # Skank / two-beat: kick on the 8th downbeats, snare on the offbeats.
    "skank_beat": dict(bpm=180, spb=4, lanes={
        "crash": "X... .... .... .... | .... .... .... ....",
        "hh":    "..x. x.x. x.x. x.x. | x.x. x.x. x.x. x.x.",
        "kick":  "x... x... x... x... | x... x... x... x...",
        "snare": "..x. ..x. ..x. ..x. | ..x. ..x. ..x. ..x.",
    }),
    # Constant 16th double kick, backbeat on 2 & 4.
    "double_kick_16ths": dict(bpm=160, spb=4, lanes={
        "crash": "X... x... x... x... | X... x... x... x...",
        "kick":  "xxxx xxxx xxxx xxxx | xxxx xxxx xxxx xxxx",
        "snare": ".... X... .... X... | .... X... .... X...",
    }),
    # Gallop double kick (8th + two 16ths).
    "double_kick_gallop": dict(bpm=150, spb=4, lanes={
        "crash": "X... .... .... .... | .... .... .... ....",
        "ride":  "..x. x.x. x.x. x.x. | x.x. x.x. x.x. x.x.",
        "kick":  "x.xx x.xx x.xx x.xx | x.xx x.xx x.xx x.xx",
        "snare": ".... X... .... X... | .... X... .... X...",
    }),
    # Mid-tempo march (Darkthrone / Bathory feel): 8th kicks, crashes on the beat.
    "midtempo_march": dict(bpm=130, spb=4, lanes={
        "crash": "X... x... x... x... | X... x... x... x...",
        "kick":  "x.x. x.x. x.x. x.x. | x.x. x.x. x.x. x.x.",
        "snare": ".... X... .... X... | .... X... .... X...",
    }),
    # Half-time atmospheric: snare on 3, open ride 8ths, sparse kick.
    "halftime_atmospheric": dict(bpm=90, spb=4, lanes={
        "crash": "X... .... .... .... | .... .... .... ....",
        "ride":  "..x. x.x. x.x. x.x. | x.x. x.x. x.x. x.x.",
        "kick":  "x... ..x. .... .... | x... ..x. x... ..x.",
        "snare": ".... .... X... .... | .... .... X... ...g",
    }),
}

FILLS = {
    # One-beat fills, all landing on the next downbeat (the groove's crash).
    "fill_snare_16ths":       dict(spb=4, lanes={"snare": "579X"}),
    "fill_snare_32nds":       dict(spb=8, lanes={"snare": "45566789", "kick": "x...x..."}),
    "fill_snare_kick_unison": dict(spb=4, lanes={"snare": "xxxX", "kick": "xxxx"}),
    "fill_toms_16ths":        dict(spb=4, lanes={"tom1": "x...", "tom2": ".x..", "tom3": "..x.", "floor": "...X"}),
    "fill_toms_32nds":        dict(spb=8, lanes={"tom1": "xx......", "tom2": "..xx....", "tom3": "....xx..", "floor": "......xX",
                                                 "kick": "x.x.x.x."}),
    "fill_snare_to_floor":    dict(spb=8, lanes={"snare": "xxxx....", "tom2": "....xx..", "floor": "......xX",
                                                 "kick": "x...x..."}),
    "fill_triplet_toms":      dict(spb=3, lanes={"snare": "x..", "tom2": ".x.", "floor": "..X", "kick": "x.."}),
    "fill_sextuplet_hhf":     dict(spb=6, lanes={"snare": "x.....", "tom1": ".x....", "tom3": "...x..", "floor": "....X.",
                                                 "kick": "..x..x"}),
    "fill_unison_stabs":      dict(spb=4, lanes={"crash": "x.xX", "kick": "x.xx", "snare": "x.xX"}),
}


# ---------------------------------------------------------------- MIDI writing

def vlq(n):
    """MIDI variable-length quantity."""
    out = [n & 0x7F]
    n >>= 7
    while n:
        out.append((n & 0x7F) | 0x80)
        n >>= 7
    return bytes(reversed(out))


def render_events(name, spb, lanes):
    """Turn a grid into (tick, order, bytes) events; returns events and total ticks."""
    rng = random.Random(zlib.crc32(name.encode()))  # reproducible per pattern
    step_ticks = PPQ // spb
    note_len = max(10, step_ticks // 2)
    events, total_steps = [], None

    for inst, grid in lanes.items():
        grid = grid.replace(" ", "").replace("|", "")
        if total_steps is None:
            total_steps = len(grid)
        if len(grid) != total_steps:
            raise ValueError(f"{name}: lane '{inst}' has {len(grid)} steps, expected {total_steps}")
        note = GM[inst]
        for i, ch in enumerate(grid):
            if ch == ".":
                continue
            vel = VEL[ch]
            if i % spb == 0 and ch in "xX":
                vel += BEAT_ACCENT
            vel = max(1, min(127, vel + rng.randint(-HUMANIZE, HUMANIZE)))
            t = i * step_ticks
            events.append((t, 1, bytes([0x90 | CHANNEL, note, vel])))
            events.append((t + note_len, 0, bytes([0x80 | CHANNEL, note, 0])))

    return events, total_steps * step_ticks


def write_midi(path, name, bpm, spb, lanes):
    events, total = render_events(name, spb, lanes)
    events.sort(key=lambda e: (e[0], e[1]))  # note-offs before note-ons at same tick

    track = bytearray()
    track += vlq(0) + b"\xFF\x03" + vlq(len(name)) + name.encode()
    track += vlq(0) + b"\xFF\x51\x03" + (60_000_000 // bpm).to_bytes(3, "big")
    track += vlq(0) + b"\xFF\x58\x04" + bytes([4, 2, 24, 8])  # 4/4
    now = 0
    for t, _, data in events:
        track += vlq(t - now) + data
        now = t
    track += vlq(max(0, total - now)) + b"\xFF\x2F\x00"

    with open(path, "wb") as f:
        f.write(b"MThd" + struct.pack(">IHHH", 6, 0, 1, PPQ))
        f.write(b"MTrk" + struct.pack(">I", len(track)) + track)


def main():
    root = os.path.join(os.path.dirname(os.path.abspath(__file__)), "midi")
    for sub, patterns, default_bpm in (("grooves", GROOVES, None), ("fills", FILLS, 180)):
        outdir = os.path.join(root, sub)
        os.makedirs(outdir, exist_ok=True)
        for name, p in patterns.items():
            bpm = p.get("bpm", default_bpm)
            fname = f"{name}_{bpm}bpm.mid"
            write_midi(os.path.join(outdir, fname), name, bpm, p["spb"], p["lanes"])
            print(f"{sub}/{fname}")


if __name__ == "__main__":
    main()
