import argparse
import sys
import json
# import os
from datetime import datetime, timezone
# from zoneinfo import ZoneInfo
# IDK if you guys care about timezone i am being extra careful and gonna do EST 
# Note: it says Z at the end meaning it should be utc so ill stick with that

from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parent))
# resolve converts relative path to an absolute path
from treasure_runner.models import GameEngine
from treasure_runner.ui.game_ui import GameUI

def set_to_dict(profile: dict) -> dict:
    # Fully set the profile in stone as a dict
    return {
        "player_name": profile.get("player_name", "Player"),
        "games_played": int(profile.get("games_played", 0)),
        "max_treasure_collected": int(profile.get("max_treasure_collected", 0)),
        "most_rooms_world_completed": int(profile.get("most_rooms_world_completed", 0)),
        "timestamp_last_played": profile.get("timestamp_last_played", "Never"),
    }

def profile_load(profile_path: Path) -> dict:
    # Either loads it if it exists or makes a new one and loads it
    if profile_path.exists():
        with profile_path.open("r", encoding="utf-8") as p:
            profile = json.load(p)
        return set_to_dict(profile)
    profile_path.parent.mkdir(parents=True, exist_ok=True)
    # .parent strips the filename and looks at the parent folder of the path/file we have
    # then we make the directory so when we try to write to it i dont get folder not found and so profile.json can live there
    player_name = input("Username: ").strip() or "Player"
    # Just sets name to player if there is an input prb
    return set_to_dict(
        {
            "player_name": player_name,
            "games_played": 0,
            "max_treasure_collected": 0,
            "most_rooms_world_completed": 0,
            "timestamp_last_played": "Never",
        }
    )

def profile_update(profile: dict, session:dict) -> dict:
    upd = set_to_dict(profile)
    # upd["games_played"] += 1 gonna handle in UI instead

    treasures_collected = session.get("treasures_collected", 0)
    upd["max_treasure_collected"] = max(upd["max_treasure_collected"], treasures_collected)

    rooms_visited = session.get("rooms_visited", 0)
    upd["most_rooms_world_completed"] = max(upd["most_rooms_world_completed"], rooms_visited)

    upd["timestamp_last_played"] = datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")
    return upd

def profile_save(profile_path: Path, profile:dict) -> None:
    profile_path.parent.mkdir(parents=True, exist_ok=True)
    # Get the folder ready to store our profile
    with profile_path.open("w", encoding="utf-8") as f:
        json.dump(profile, f, indent=4)
        f.write("\n")


def main():
    parser = argparse.ArgumentParser(description="Treasure runner")
    parser.add_argument("--config", required=True, help="Path to the config file (.ini)")
    parser.add_argument("--profile", required=True, help="Path to json player profile")
    args = parser.parse_args()
    config_path = Path(args.config).resolve()
    profile_path = Path(args.profile).resolve()
    profile = profile_load(profile_path)
    engine = GameEngine(str(config_path))

    ui = GameUI(engine, profile)

    session = None
    try:
        session = ui.run()
    finally:
        if session is None:
            session = {
                "treasures_collected": engine.player.get_collected_count(),
                "rooms_visited" : engine.visited_room_count,
            }
        updated_prof = profile_update(profile, session)
        profile_save(profile_path, updated_prof)
        engine.destroy()
    return 0

if __name__ == "__main__":
    main()