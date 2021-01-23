# is_server_busy

Darwin tool to integrate to **autosuspend** and control shutdown of server

## Introduction

The tool is basically used to track CPU and disk usage of a given set of processes with their thresholds.

It was developed as an alternate python script that previously worked on Linux cannot operate on OS X, as the python library does not offer the same functionality found on Linux.

One would integrate this tool to the **autosuspend** (https://github.com/languitar/autosuspend) tool to control system power off.

## General Compile Instructions

Compile using the provided Makefile. Base compile system provided by MacPorts.
The **jsoncpp** library shall be also installed with MacPorts.

The following ports needs to be installed:
* clang-10
* jsoncpp
* lldb-10
* and maybe more...

Please ensure you have ``/opt/local/bin`` on your path, so MacPorts will work ok.

**Note:** I don't use **brew** as I am used to MacPorts, but surely it has the equivalent packages, as long as you fix makefile to work with.

## Debugging

miDebugger can be used from default VSCode or Apple XCode as provided in the docs from Microsoft. I had real trouble debugging STL and a weird behavior of double source code views, related to paths, which I was unable to circumvent.

Then I stumbled across plugin **lanza.lldb-vscode** which uses the official mi debugger from LLVM project, called **lldb-vscode**.
This is installed by MacPort on ``/opt/local/bin/lldb-vscode-mp-10``.

## Command Line Usage

```
$ ./output/is_server_busy -h
is_server_busy
==============
Tool to track service activity, to be used with autosuspend.
USAGE: is_server_busy [-h] [-v] [-c <config>] [-l <log-file>] [-L <level>]
    -c <config>           : specify a configuration file. Default to
                            '/opt/local/etc/is_server_busy.conf'.
    -h, --help            : show help
    -l <log-file>         : Same as option --log-file
    --log-file=<log-file> : Specifies a log file
    -L <level>            : Specifies the log level. Allowed values are
                            ERROR,WARN,INFO or DEBUG.
    -v                    : Increase verbosity
```



