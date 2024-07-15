import unittest

from services.stopwatch.stop_stopwatch import StopStopwatch
from services.stopwatch.stop_stopwatch_pb2 import StopStopwatchParams


class StopStopwatchTest(unittest.TestCase):

    def test_create_skill(self):
        skill = StopStopwatch()


if __name__ == '__main__':
    unittest.main()
