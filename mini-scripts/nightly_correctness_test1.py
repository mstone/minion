#!/usr/bin/python

# Script to run the nightly correctness testing scripts.

from os import *
from sys import exit
import sendemail

# of course this is bad. The minion directory should be a command line option.
homedir="/cs/home/rsch/pn/"
miniondir="minion-svn1/minion"

chdir(homedir+miniondir)   #cd /home/pn/minion-svn/minion
system("rm Makefile*")   # get rid of stale makefiles that were causing problems.

retval=0
retval+=system("svn update")
retval+=system("./configure.sh")
retval+=system("make veryclean")

if retval!=0:
    sendemail.mail("An error occurred when checking out or doing make veryclean.")
    exit(1)

retval+=system("make minion -j2")

if retval!=0:
    sendemail.mail("An error occurred when building minion.")
    exit(1)

retval+=system("make minion DEBUG=1 -j2")

if retval!=0:
    sendemail.mail("An error occurred when building minion-debug.")
    exit(1)

chdir("test_instances")
retval+=system("./run_tests.sh ../bin/minion")

if retval!=0:
    sendemail.mail("An error occurred when running test_instances/run_tests.sh for minion")
    exit(1)

chdir("test_instances")
retval+=system("./run_tests.sh ../bin/minion-debug")

if retval!=0:
    sendemail.mail("An error occurred when running test_instances/run_tests.sh for minion-debug")
    exit(1)

chdir(homedir+miniondir)   #cd /home/pn/minion-svn1/minion

system("mini-scripts/testallconstraints.py --numtests=500 --email --minion=bin/minion &")
system("mini-scripts/testallconstraints.py --numtests=100 --email --minion=bin/minion-debug")

