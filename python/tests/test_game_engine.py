import unittest
from treasure_runner.models.game_engine import GameEngine
from treasure_runner.models.exceptions import ImpassableError, InvalidArgumentError
from treasure_runner.bindings import Direction

CONFIG_FILE = "example_integration_run.ini"

class TestPlayerFunctions(unittest.TestCase):

    def setUp(self):
        self.engine = GameEngine(CONFIG_FILE)

    def tearDown(self):
        self.engine.destroy
    
    def test_player_none(self):
        self.assertIsNotNone(self.engine.player)
    
    def test_get_room_restype(self):
        room = self.engine.player.get_room()
        self.assertIsInstance(room, int)
    
    def test_get_pos_restype(self):
        pos = self.engine.player.get_position()
        self.assertIsInstance(pos, tuple)
        self.assertEqual(len(pos), 2)
    
    def test_get_collected_treasure(self):
        result = self.engine.player.get_collected_treasures()
        self.assertIsInstance(result, list)
        self.assertEqual(len(result), 0)

    def test_init_get_collected_count(self):
        self.assertEqual(self.engine.player.get_collected_count(), 0)
    
    def test_has_collected_treasure(self):
        self.assertFalse(self.engine.player.has_collected_treasure(1))

class TestMovement(unittest.TestCase):
    def setUp(self):
        self.engine = GameEngine(CONFIG_FILE)

    def tearDown(self):
        self.engine.destroy
    
    def test_pos_and_room_changes(self):
        prev_pos = self.engine.player.get_position()
        prev_room = self.engine.player.get_room()
        self.engine.move_player(Direction.NORTH)
        self.assertNotEqual(prev_pos, self.engine.player.get_position())
        self.assertEqual(prev_room, self.engine.player.get_room())

class TestGameEngine:
    def setUp(self):
        self.engine = GameEngine(CONFIG_FILE)

    def tearDown(self):
        self.engine.destroy

    def test_room_stat_functions(self):
        self.assertGreater(self.engine.get_room_count(), 0)
        w, h = self.engine.get_room_dimensions()
        self.assertGreater(w, 0)
        self.assertGreater(h, 0)
        ids = self.engine.get_room_ids()
        self.assertIsInstance(ids, list)
        self.assertEqual(len(ids), self.engine.get_room_count())
    
    def test_render_current_room(self):
        res = self.engine.render_current_room()
        self.assertIsInstance(res, str)
    
    def test_reset(self):
        start = self.engine.player.get_position()
        self.engine.move_player(Direction.NORTH)
        self.engine.reset()
        self.assertEqual(self.engine.player.get_position(), start)
    

if __name__ == "__main__":
    unittest.main()
