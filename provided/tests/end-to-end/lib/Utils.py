import re

class Utils:
    def __init__(self, exe):
        self.exe = exe

    def convert_regexp(self, regex):
        return f"{re.escape(self.exe)} <disk> {regex}"
        