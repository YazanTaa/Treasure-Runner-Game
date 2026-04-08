# from __future__ import annotations
# For forward referencing
import curses
from ..models.game_engine import GameEngine
from ..bindings import Direction
from ..models.exceptions import GameError, ImpassableError, GameEngineError

class GameUI:
    def __init__(self, engine: GameEngine, profile: dict, title: str="Yazan's Treasure Game", email: str = "ytaamneh@uoguelph.ca"):
        self._eng = engine
        self._profile = profile
        self._title = title
        self._email = email
        self._msg = "GOAL: Find all the treasures to win"
        self._curr = "playing"

    def run(self) -> dict:
        return curses.wrapper(self._main)

# EXTENDED FEATURE COLOURS:
    def init_colours(self):
        if not curses.has_colors():
            return
        curses.start_color()
        curses.use_default_colors()
        curses.init_pair(1, curses.COLOR_YELLOW, -1) #treaure yellow
        curses.init_pair(2, curses.COLOR_CYAN, -1) #player cyan
        curses.init_pair(3, curses.COLOR_MAGENTA, -1) #portals purple
        curses.init_pair(4, curses.COLOR_RED, -1) # walls red
        curses.init_pair(5, curses.COLOR_GREEN, -1) #message green maybe do white and PUSHABLES NOW GREEN
        curses.init_pair(6, curses.COLOR_BLUE, -1) #status blue
# MAIN
    def _main(self, stdscr) -> dict:
        curses.curs_set(0)
        self.init_colours()
        height, width = stdscr.getmaxyx()
        if height<24 or width < 60:
            curses.endwin()
            raise RuntimeError(f"TERMINAL TOO SMALL: MUST BE AT LARGER THAN 24x60, CURRENTLY {width}x{height}")

        stdscr.keypad(True)
        stdscr.nodelay(False)

        self.show_stats(stdscr, "WELCOME TO MY GAME")
        # stdscr.clear()
        # self.draw_game(stdscr)

        while self._curr == "playing":
            self.draw_game(stdscr)
            self.input_handler(stdscr)

        session = {
            "treasures_collected": self._eng.player.get_collected_count(),
            "rooms_visited": self._eng.visited_room_count,
        }
        # Update stats before displaying the screen
        self._profile["games_played"] += 1
        self._profile["max_treasure_collected"] = max(self._profile["max_treasure_collected"],session["treasures_collected"])
        self._profile["most_rooms_world_completed"] = max(self._profile["most_rooms_world_completed"], session["rooms_visited"])
        # Vistory or give up screen
        if self._curr == "won":
            self.show_stats(stdscr, "VICTORY!")
        else:
            self.show_stats(stdscr, "GAME OVER")
        return session
# SCREENS

    def show_stats(self, stdscr, title: str):
        stdscr.clear()
        height,width = stdscr.getmaxyx()
        lines = [title, "",
                 f"Player: {self._profile['player_name']}",
                 f"Games played: {self._profile['games_played']}",
                 f"Most treasures collected: {self._profile['max_treasure_collected']}",
                 f"Most rooms explored: {self._profile['most_rooms_world_completed']}",
                 f"Last played: {self._profile['timestamp_last_played']}", "",
                 "Press any key to continue",
                 ]
        for i, line in enumerate(lines):
            y= max(0, height//2 - len(lines)//2) + i
            x = max(0, width // 2 - len(line) // 2)
            if y<height and (x+len(line))<width:
                stdscr.addstr(y,x, line)
        stdscr.refresh()
        stdscr.getch()

# INPUT HANDLING:
    def movement_handler(self, key, key_mapping):
        if key not in key_mapping:
            return
        prev_treasures = self._eng.player.get_collected_count()
        try:
            self._eng.move_player(key_mapping[key])
            collected_treasures = self._eng.player.get_collected_count()
            total_treasures = self._eng.get_treasure_count()
            if prev_treasures < collected_treasures:
                self._msg = f"Treasure Collected: {collected_treasures}/{total_treasures}"
            else:
                self._msg = "GOAL: Find all the treasures to win"
            if self._eng.game_won():
                self._curr = "won"
        except ImpassableError:
            self._msg = "Can't go through walls"
        except GameEngineError as error_msg:
            self._msg = str(error_msg)

    def input_handler(self, stdscr):
        key_mapping = {
            curses.KEY_UP: Direction.NORTH,
            curses.KEY_DOWN: Direction.SOUTH,
            curses.KEY_LEFT: Direction.WEST,
            curses.KEY_RIGHT: Direction.EAST,
            ord('w'): Direction.NORTH,
            ord('W'): Direction.NORTH,
            ord('s'): Direction.SOUTH,
            ord('S'): Direction.SOUTH,
            ord('a'): Direction.WEST,
            ord('A'): Direction.WEST,
            ord('d'): Direction.EAST,
            ord('D'): Direction.EAST,
        }

        key = stdscr.getch()

        if key == ord('q'):
            self._curr = "quit"
            return

        if key == ord('r'):
            self._eng.reset()
            self._msg = "GAME HAS BEEN RESET"
            return
        # CURRENTLY WORKING BUT NOT SURE IF MUST BE ON PORTAL
        if key == ord('>'):
            prev = self._eng.player.get_room()
            prev_x, prev_y = self._eng.player.get_position()
            for move_dir in Direction:
                try:
                    self._eng.move_player(move_dir)
                    if self._eng.player.get_room() != prev:
                        self._msg = "PORTALED"
                        return
                    else:
                        self._eng.player.set_position(prev_x, prev_y)
                except GameEngineError:
                    pass
            self._msg = "No Portal to use"
            return
        if key in key_mapping:
            self.movement_handler(key, key_mapping)
    def fill_colours(self, char:str) -> int:
        if char == '$':
            return curses.color_pair(1)
        if char == '@':
            return curses.color_pair(2)
        if char == 'X':
            return curses.color_pair(3)
        if char == '#':
            return curses.color_pair(4)
        if char == 'O':
            return curses.color_pair(5)
        return curses.color_pair(0) #should be white/black depending on background by default

    def draw_map_node(self, stdscr, y: int, x: int, room_id: int, row: list, ids: list, width: int):
        node = f"{room_id:2d}"
        # if x+len(node) < width:
        #     stdscr.addstr(y,x,node)

        # red connections to the right of a node
        x += len(node)+1
        for j, is_connected in enumerate(row):
            if is_connected:
                link = f"-{ids[j]}"
                if x+len(link) < width:
                    stdscr.addstr(y,x,link,curses.color_pair(4))
                    x+= len(link)+1

    def draw_minimap(self, stdscr, starting_y: int, starting_x: int):
        height, width = stdscr.getmaxyx()
        try:
            mtx, ids = self._eng.get_adj_mtx()
        except GameEngineError:
            return
        room = self._eng.player.get_room()
        visited_rooms = self._eng.visited_rooms

        stdscr.addstr(starting_y, starting_x, "MINIMAP:", curses.color_pair(5))

        for i, room_id in enumerate(ids):
            y = starting_y +1+i
            if y>= height-2:
                break
            if room_id == room:
                colour = curses.color_pair(2) #cyan is current
            elif room_id in visited_rooms:
                colour = curses.color_pair(3) #purple visited alr
            else:
                colour = curses.color_pair(0) #unvisited is just default
            stdscr.addstr(y, starting_x, f"{room_id:2d}",colour)
            self.draw_map_node(stdscr, y, starting_x, room_id, mtx[i], ids, width)

    def draw_game(self, stdscr):
        stdscr.erase()
        height,width = stdscr.getmaxyx()


        stdscr.addstr(0,0, self._msg[:width -1], curses.color_pair(1))
        # msg at row 0

        room_id = self._eng.player.get_room()
        stdscr.addstr(1,0, f"Room {room_id}"[:width-1], curses.color_pair(4))
        # room id on row 1 maybe color it in later

        room_bfr = self._eng.render_current_room()
        rows = room_bfr.split('\n')
        starting_y = 2
        for i, row in enumerate(rows):
            y = starting_y + i
            if y>=height-4:
                break
            for x, char in enumerate(row):
                if x>=width-1:
                    break
                try:
                    stdscr.addch(y,x,char,self.fill_colours(char))
                except curses.error:
                    pass

        # Legend showing controls and guiding
        legend_y = starting_y + len(rows) + 1
        if legend_y < height-3:
            stdscr.addstr(legend_y, 0, "Game Elements: @ player, $ gold, X portal, # wall, O pushable"[:width-1])
        if legend_y + 1 < height - 3:
            stdscr.addstr(legend_y+1, 0,"Controls: WASD/Arrows to move | > to portal | r to reset | q to quit"[:width-1])

        #Details of player
        collected = self._eng.player.get_collected_count()
        total = self._eng.get_treasure_count()
        visited = self._eng.visited_room_count
        n_rooms = self._eng.get_room_count()
        name = self._profile.get("player_name", "Player")
        status = f"Username:{name} | Treasure: {collected}/{total} | Rooms: {visited}/{n_rooms} "
        if height-2 > 0:
            stdscr.addstr(height-2, 0, status[:width-1], curses.color_pair(6))

        # Title
        title = f"{self._title} | {self._email}"
        if height-1>0:
            stdscr.addstr(height-1, 0, title[:width-1])
        self.draw_minimap(stdscr, 2, width-25)
        stdscr.refresh()
