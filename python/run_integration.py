#!/usr/bin/env python3
"""Deterministic system integration test runner for Treasure Runner."""

# below are the imports that are in the instructors solution.
# you may need different ones.  Use them if you wish.
import os
import argparse
from treasure_runner.bindings import Direction
from treasure_runner.models.game_engine import GameEngine
from treasure_runner.models.exceptions import GameError, ImpassableError


# put your functions here here


def parse_args():
    parser = argparse.ArgumentParser(description="Treasure Runner integration test logger")
    parser.add_argument(
        "--config",
        required=True,
        help="Path to generator config file",
    )
    parser.add_argument(
        "--log",
        required=True,
        help="Output log path",
    )
    return parser.parse_args()

# Current player room coords and colleced ammount
def get_player_state(engine):
    player = engine.player
    room = player.get_room()
    x,y = player.get_position()
    collected_count = player.get_collected_count()
    return room, x, y, collected_count

# Turn player state into a string sepeated by | to be parsed
def state_string(room, x, y, collected):
    return f"room={room}|x={x}|y={y}|collected={collected}"

#  SOUTH, WEST, NORTH, EAST exact order
# Room id is same but coords diff
# Only check attempted dir that doesnt raise ImpassableErr
def get_first_move(engine, config_path):
    for direction in [Direction.SOUTH, Direction.WEST, Direction.NORTH, Direction.EAST]:
        prev_room, prev_x, prev_y, _ = get_player_state(engine)
        try:
            engine.move_player(direction)
            nxt_room, nxt_x, nxt_y, _ = get_player_state(engine)
            if nxt_room == prev_room and (nxt_x!= prev_x or nxt_y!=prev_y):
                engine.reset()
                return direction
        except ImpassableError:
            pass
        engine.reset()
    raise RuntimeError("No valid first direction found")

# String dir
def get_dir_name(direction):
    return direction.name

def run_integration(config_path, log_path):
    engine = GameEngine(config_path)
    lines =[]

    room_count = engine.get_room_count()
    width, height = engine.get_room_dimensions()

    lines.append(f"RUN_START|config={config_path}|rooms={room_count}" f"|room_width={width}|room_height={height}")
    room, x, y, collected = get_player_state(engine)
    lines.append(f"STATE|step=0|phase=SPAWN|state={state_string(room, x, y, collected)}")
    first_move = get_first_move(engine, config_path)
    lines.append(f"ENTRY|direction={get_dir_name(first_move)}")

    engine.reset()
    step=0

    prev_state = get_player_state(engine)
    step+=1
    try:
        engine.move_player(first_move)
        nxt_state = get_player_state(engine)
        result = "OK"
    # Impassible means blocked
    except ImpassableError:
        nxt_state = prev_state
        result = "BLOCKED"
    # Any non-impassable game error on move results in logging ERROR and ending the phase as blocked/no progress
    except Exception:
        nxt_state = prev_state
        result = "ERROR"
    
    delta = nxt_state[3] - prev_state[3]
    lines.append(
        f"MOVE|step={step}|phase=ENTRY|dir={get_dir_name(first_move)}|result={result}"
        f"|before={state_string(*prev_state)}|after={state_string(*nxt_state)}"
        f"|delta_collected={delta}"
    )
    # Deal with ERROR immediately to terminate
    if result == "ERROR":
        lines.append("TERMINATED: Initial Move Error")
        lines.append(f"RUN_END|steps={step}|collected_total={nxt_state[3]}")
        with open(log_path, "w", encoding="utf-8") as log_file:
            log_file.write("\n".join(lines) + "\n")
        engine.destroy()
        return 0
    sweeps = [(Direction.SOUTH, "SWEEP_SOUTH"), (Direction.WEST,  "SWEEP_WEST"), (Direction.NORTH, "SWEEP_NORTH"), (Direction.EAST,  "SWEEP_EAST"),]

    for sweep_dir, phase_str in sweeps:
        lines.append(f"SWEEP_START|phase={phase_str}|dir={get_dir_name(sweep_dir)}")
        sweep_count = 0
        # Set of states weve alr seen
        past_states = set()
        reason = "BLOCKED"

        while True:
            prev_state = get_player_state(engine)
            state_content = (prev_state[0], prev_state[1], prev_state[2], prev_state[3])
            if state_content in past_states:
                reason = "CYCLE_DETECTED"
                nxt_state = prev_state
                delta = 0
                step += 1
                lines.append( f"MOVE|step={step}|phase={phase_str}|dir={get_dir_name(sweep_dir)}"
                    f"|result=BLOCKED|before={state_string(*prev_state)}"
                    f"|after={state_string(*nxt_state)}|delta_collected={delta}")
                break
            past_states.add(state_content)
            step+=1
            try:
                engine.move_player(sweep_dir)
                nxt_state = get_player_state(engine)
                if nxt_state[:3] == prev_state[:3] and nxt_state[3] == prev_state[3]:
                    result = "BLOCKED"
                    reason = "BLOCKED"
                    delta = 0
                    lines.append(f"MOVE|step={step}|phase={phase_str}|dir={get_dir_name(sweep_dir)}" 
                                 f"|result={result}|before={state_string(*prev_state)}"
                                f"|after={state_string(*nxt_state)}|delta_collected={delta}")
                    break
                result = "OK"
                delta = nxt_state[3] - prev_state[3]
                lines.append(f"MOVE|step={step}|phase={phase_str}|dir={get_dir_name(sweep_dir)}"
                            f"|result={result}|before={state_string(*prev_state)}"
                            f"|after={state_string(*nxt_state)}|delta_collected={delta}")
                sweep_count +=1
            except ImpassableError:
                nxt_state = prev_state
                result = "BLOCKED"
                reason = "BLOCKED"
                delta = 0
                lines.append(f"MOVE|step={step}|phase={phase_str}|dir={get_dir_name(sweep_dir)}"
                            f"|result={result}|before={state_string(*prev_state)}"
                            f"|after={state_string(*nxt_state)}|delta_collected={delta}")
                break
            except Exception:
                nxt_state = prev_state
                result = "ERROR"
                reason = "BLOCKED"
                delta = 0
                lines.append(f"MOVE|step={step}|phase={phase_str}|dir={get_dir_name(sweep_dir)}"
                            f"|result={result}|before={state_string(*prev_state)}"
                            f"|after={state_string(*nxt_state)}|delta_collected={delta}")
                break
        lines.append(f"SWEEP_END|phase={phase_str}|reason={reason}|moves={sweep_count}")
    fin_state = get_player_state(engine)
    lines.append(
        f"STATE|step={step}|phase=FINAL|state={state_string(*fin_state)}"
    )
    lines.append(f"RUN_END|steps={step}|collected_total={fin_state[3]}")

    with open(log_path, "w", encoding="utf-8") as log_file:
        log_file.write("\n".join(lines)+"\n")
    engine.destroy()

    return 0

                    



def main():
    args = parse_args()
    config_path = os.path.abspath(args.config)
    log_path = os.path.abspath(args.log)
    #call your main function here
    return run_integration(config_path, log_path)


if __name__ == "__main__":
    raise SystemExit(main())
