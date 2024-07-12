import unittest

from services.stopwatch.start_stopwatch import StartStopwatch
from services.stopwatch.start_stopwatch_pb2 import StartStopwatchParams


class StartStopwatchTest(unittest.TestCase):

    def test_create_skill(self):
        skill = StartStopwatch()


if __name__ == '__main__':
    unittest.main()
