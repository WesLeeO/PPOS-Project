# Author: Aurélien (2021)
# Modified by Ludovic Mermod

import os
import re
import shlex
import time

from robot.libraries.Process import Process
from robot.libraries.BuiltIn import BuiltIn
from Errors import Errors

def log_crash_if_any(res):
    # Check if a crash occurred, and if so, print the stacktrace
    if res.rc == 127 and 'error while loading shared libraries' in res.stdout:
        raise Exception('You are missing an export LD_LIBRARY_PATH:\n{}'.format(res.stdout))
    elif res.rc != 0 and U6fsUtils.ASAN_CRASH_REGEXP.search(res.stdout):
        raise Exception('A crash occurred. Here is the output:\n{}'.format(res.stdout))


class U6fsUtils:
    """
     Utils specific to the ckvs project
     """
    ROBOT_LIBRARY_SCOPE = 'SUITE'
    ROBOT_LISTENER_API_VERSION = 2

    ASAN_CRASH_REGEXP = re.compile(r'==\d+==\s*ERROR:')
    """ Used to determine whether a crash occurred. """

    def __init__(self, exec_path: str, enum_filename: str, enum_name: str, error_filename: str, error_array_name: str, **kwargs):
        self.process = Process()
        self.builtin = BuiltIn()
        self.errors = Errors(enum_filename, enum_name, error_filename, error_array_name, **kwargs)
        self.executable = exec_path

        # register self as listener to log the commands that were executed
        self.ROBOT_LIBRARY_LISTENER = self
        self.logged_commands = []

        self.fuse = None

    def _start_test(self, name, attributes):
        """ Initializes the library when a test case starts. """
        self.logged_commands = []

    def _end_test(self, name, attributes):
        """ Prints a message to help debugging if the test case failed. """
        always_print = False
        if always_print or attributes['status'] == 'FAIL':
            self.builtin.log_to_console('\n*** To recreate the test case, run the following command(s):')
            for cmd in self.logged_commands:
                self.builtin.log_to_console(cmd)

    def u6fs_run(self, *args, background=False, expected_ret=None, expected_string=None, expected_file=None,
                 expected_regexp=None):

        """
        Run the u6fs executable and gives the arguments as parameters.

        :param background  Runs the executable in background. No checks on the output are done (expected_ret,
        expected_string, expected_file expected_regexp are ignored)

        :param expected_ret    The variant of the error enum expected as a return code. If None, no check is done

        :param expected_string The expected output. If None, no check is done

        :param expected_file   The file containing the expected output. If None, no check is done

        :param expected_regexp  A regexp that should match the output. The regex may not match the whole output, use  "^$".
        If None, no check is done
        """

        # record the command
        self.logged_commands.append(shlex.join([os.path.basename(self.executable), *args]))

        if background:
            return self.process.start_process(self.executable, *args, stderr="STDOUT")
        else:
            res = self.process.run_process(self.executable, *args, stderr="STDOUT")
            log_crash_if_any(res)

            if expected_ret:
                self.errors.compare_exit_code(res, expected_ret)

            if expected_string:
                self.builtin.should_be_equal(res.stdout.strip(), expected_string.strip())

            if expected_file:
                file = open(expected_file).read()
                self.builtin.should_be_equal(res.stdout.strip(), file.strip())

            if expected_regexp:
                self.builtin.should_match_regexp(res.stdout, expected_regexp)

            return res

    def u6fs_sb(self, filename):
        return self.u6fs_run(filename, "sb")

    def u6fs_inode(self, filename):
        return self.u6fs_run(filename, "inode")

    def u6fs_cat1(self, filename, inr):
        return self.u6fs_run(filename, "cat1", inr)

    def u6fs_tree(self, filename):
        return self.u6fs_run(filename, "tree")

    def u6fs_sha(self, filename):
        return self.u6fs_run(filename, "shafiles")

    def u6fs_start_fuse(self, filename, mountpoint):
        self.u6fs_stop_fuse(mountpoint, check=False)

        if not os.path.exists(mountpoint):
            os.mkdir(mountpoint)

        self.fuse = self.u6fs_run(filename, "fuse", mountpoint, background=True)
        time.sleep(0.1)
        return self.fuse

    def u6fs_stop_fuse(self, mountpoint, check=True):
        self.logged_commands.append(shlex.join(["fusermount", "-u", mountpoint]))
        ret = self.process.run_process("fusermount", "-u", mountpoint, stderr="STDOUT")

        if check and ret.rc != 0:
            raise Exception(f"fusermount exited with {ret.rc} instead of 0")

        if self.fuse:
            return self.process.wait_for_process(self.fuse)

    def u6fs_bm(self, filename):
        return self.u6fs_run(filename, "bm")

    def u6fs_mkdir(self, filename, dir):
        return self.u6fs_run(filename, "mkdir", dir)

    def u6fs_add(self, filename, dest, src):
        return self.u6fs_run(filename, "add", dest, src)

    def u6fs_create_dump(self, filename, dumpname):
        res = self.process.run_process('cp', filename, dumpname)
        self.logged_commands.append(shlex.join(['cp', filename, dumpname]))
        if res.rc != 0:
            raise Exception(f"could not create dump file ({res.rc})")
