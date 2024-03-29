#!/bin/bash

gcc -I/usr/include/libnl3 generic_user.c -lnl-3 -lnl-genl-3 -o generic_user
