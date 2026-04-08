import ctypes
from ..bindings import lib, Treasure


class Player:
    def __init__(self, ptr):
        self._ptr = ptr

    def get_room(self) -> int:
        # Room id
        return lib.player_get_room(self._ptr)

    def get_position(self) -> tuple[int, int]:
        # x and y pos
        x = ctypes.c_int()
        y = ctypes.c_int()
        lib.player_get_position(self._ptr, ctypes.byref(x), ctypes.byref(y))
        return x.value, y.value

    def get_collected_count(self) -> int:
        # num collected treasures
        return lib.player_get_collected_count(self._ptr)

    def has_collected_treasure(self, treasure_id: int) -> bool:
        # Has treasure been collected
        return lib.player_has_collected_treasure(self._ptr, treasure_id)

    def get_collected_treasures(self) -> list[dict]:
        # Collected treasures as dicts/hash
        count = ctypes.c_int()
        arr = lib.player_get_collected_treasures(
            self._ptr, ctypes.byref(count))
        temp = []
        for i in range(count.value):
            treasure = arr[i].contents
            temp.append({
                "id": treasure.id,
                "name": treasure.name.decode("utf-8") if treasure.name else "",
                "starting_room_id": treasure.starting_room_id,
                "initial_x": treasure.initial_x,
                "initial_y": treasure.initial_y,
                "x": treasure.x,
                "y": treasure.y,
                "collected": treasure.collected,
            })
        return temp
    def set_position(self, x, y):
        # status = 
        lib.player_set_position(self._ptr, x, y)
        # if status != Status.OK:
        #     raise status_to_exception(Status(status), "Set pos fail")
