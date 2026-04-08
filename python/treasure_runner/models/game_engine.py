import ctypes
from ..bindings import lib, Direction, Status, GameEngine as CGameEngine
from .exceptions import GameEngineError, status_to_exception
from .player import Player


class GameEngine:
    def __init__(self, config_path: str):
        self._eng = CGameEngine()
        status = lib.game_engine_create(
            config_path.encode("utf-8"), ctypes.byref(self._eng))
        if status != Status.OK:
            raise status_to_exception(Status(status), "Create Failed")
        player_ptr = lib.game_engine_get_player(self._eng)
        if not player_ptr:
            raise RuntimeError("Get player returned NULL")
        self._player = Player(player_ptr)
        self._visited_rooms = {self.player.get_room()}

    @property
    def player(self) -> Player:
        return self._player

    def destroy(self) -> None:
        if self._eng:
            lib.game_engine_destroy(self._eng)
            self._eng = None

    def move_player(self, direction: Direction) -> None:
        status = lib.game_engine_move_player(self._eng, int(direction))
        if status != Status.OK:
            raise status_to_exception(Status(status), "Move player failed")
        self._visited_rooms.add(self.player.get_room())

    def render_current_room(self) -> str:
        buffer = ctypes.c_char_p()
        status = lib.game_engine_render_current_room(
            self._eng, ctypes.byref(buffer))
        if status != Status.OK:
            raise status_to_exception(Status(status), "render failed")
        r_val = buffer.value.decode("utf-8")
        lib.game_engine_free_string(buffer)
        return r_val

    def get_room_count(self) -> int:
        count = ctypes.c_int(0)
        status = lib.game_engine_get_room_count(self._eng, ctypes.byref(count))
        if status != Status.OK:
            raise status_to_exception(Status(status), "get room count failed")
        return count.value

    def get_room_dimensions(self) -> tuple[int, int]:
        width = ctypes.c_int()
        height = ctypes.c_int()
        status = lib.game_engine_get_room_dimensions(
            self._eng, ctypes.byref(width), ctypes.byref(height))
        if status != Status.OK:
            raise status_to_exception(Status(status))
        return width.value, height.value

    def get_room_ids(self) -> list[int]:
        id_pointer = ctypes.POINTER(ctypes.c_int)()
        count = ctypes.c_int()
        status = lib.game_engine_get_room_ids(
            self._eng, ctypes.byref(id_pointer), ctypes.byref(count))
        if status != Status.OK:
            raise status_to_exception(Status(status), "get room ids failed")
        id_list = [id_pointer[i] for i in range(count.value)]
        lib.game_engine_free_string(id_pointer)
        return id_list

    def reset(self) -> None:
        status = lib.game_engine_reset(self._eng)
        if status != Status.OK:
            raise status_to_exception(Status(status), "reset failed")
        self._visited_rooms = {self.player.get_room()}
# A3 functions
    def get_treasure_count(self) -> int:
        count = ctypes.c_int(0)
        status = lib.game_engine_get_treasure_count(self._eng, ctypes.byref(count))
        if status != Status.OK:
            raise status_to_exception(Status(status))
        return count.value
    def game_won(self) -> bool:
        return self._player.get_collected_count() == self.get_treasure_count()
    # def get_current_room_id(self) -> int:
    #     room_id = ctypes.c_int()
    #     status = lib.game_engine_get_current_room
    @property
    def visited_rooms(self) -> set[int]:
        return set(self._visited_rooms)
    @property
    def visited_room_count(self)->int:
        return len(self.visited_rooms)
    def get_adj_mtx(self) -> tuple[list[list[int]], list[int]]:
        mtx_ptr = ctypes.POINTER(ctypes.c_int)()
        count = ctypes.c_int(0)
        status = lib.game_engine_get_adj_mtx( self._eng, ctypes.byref(mtx_ptr), ctypes.byref(count))
        if status!= Status.OK:
            raise status_to_exception(Status(status))
        mtx_size = count.value
        mtx = [[mtx_ptr[i*mtx_size+j]for j in range(mtx_size)] for i in range(mtx_size)]
        ids = self.get_room_ids()
        lib.game_engine_free_string(mtx_ptr)
        return mtx, ids
