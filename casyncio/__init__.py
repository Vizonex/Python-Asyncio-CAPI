"""
C-Asyncio
---------

An external CPython / Cython Library for making asyncio
run smoother in CPython and Cython
"""

from typing import Union
from pathlib import Path
import json
import os

__author__ = "Vizonex"
__version__ = "0.1.0"
__license__ = "Appache 2.0"


def copy_header_file(output: Union[str, Path]) -> int:
    r"""
    Copies the c-asyncio Header file
    good for when you need to compile
    with CPython or When You need a place
    for tools like Visual Studio Code to
    find and read the header file you want
    to utilize may not be as important in Cython
    but is useful for to have if reviewing Cython
    code.
    """
    header_file = Path(*__path__) / "casyncio.h"
    return Path(output).write_text(header_file.read_text("utf-8"), "utf-8")


# Always felt annoying back in the day when I was a noob at this myself
# and having to figure out why the application could never find
# Python.h and you want to use auto-suggestions for these projects.
# So here is a little tool / python function you can use to help you
# point Intellisense in the right direction, your welcome ^^

# wording of this function was inspired by the colorama python library.


def just_fix_vscode_c_cpp_properties_file(vs_code_dir: Union[str, Path] = ".vscode") -> int:
    r"""
    Fixes or creates a new `.vscode/c_cpp_properties.json`
    file in vs-code to point to Python.h & casyncio.h files to prove they
    are not missing and to make the process of finding these
    items a bit less difficult (especially for newer users)

    returns the status code after writing from the file
    """
    json_file = Path(vs_code_dir) / "c_cpp_properties.json"
    # VS-Code can get a little picky so you should
    # always use posix for the configuration
    cpython_include_path = (Path(os.environ["PYTHONPATH"]) / "include").as_posix()
    casyncio_include_path = Path(*__path__).as_posix()

    if not json_file.exists():
        if not os.path.exists(vs_code_dir):
            os.mkdir(vs_code_dir)
        return json_file.write_text(
            json.dumps(
                {
                    "configurations": {
                        "includePath": [
                            "${workspaceFolder}/**",
                            cpython_include_path,
                            casyncio_include_path,
                        ]
                    }
                },
                indent=4
            )
        )

    else:
        # take an already configured json file and just add to it the items we need...
        items = json.load(json_file.read_bytes())
        paths: list[str] = items["configurations"]["includePath"]
        if cpython_include_path not in paths:
            paths.append(cpython_include_path)
        if casyncio_include_path not in paths:
            paths.append(casyncio_include_path)
        return json_file.write_text(json.dumps(paths, sort_keys=True, indent=4))
