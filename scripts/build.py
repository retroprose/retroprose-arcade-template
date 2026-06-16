import sys
import json
import shutil
import pathlib
import subprocess

data = {}
flags = ['-profiling-funcs', '-Iinclude', '-std=c++17']
objectflags = ['-c']
targetflags = ['-lembind', '-sASSERTIONS', '-sSTACK_SIZE=5000000', '-sALLOW_MEMORY_GROWTH']

def build(key_path):
    # check if this has already be set to build
    if key_path in data:
        return

    # load data from source folder
    buildjson = pathlib.Path(f"./{key_path}/build.json")
    if buildjson.is_file() == False:
        print(f"Skipping {key_path}, build.json does not exist.")
        return

    # load data into global dictionary
    data[key_path] = json.load(open(buildjson.resolve()))

    # if this is simply a header library nothing needs to be done
    if data[key_path]['target'] == '':
        return

    # build dependencies, and make list of library flies
    libs = []
    for item in data[key_path]['dependencies']:
        build('libs/' + item)
        if data['libs/' + item]['target'] == 'lib':
            libs += [f"./libs/{item}/build/{item}.a"]

    # get name without path
    key = key_path.split('/')[1]

    # clean build folder and final output
    buildpath = pathlib.Path(f"./{key_path}/build")
    buildpath.mkdir(parents=True, exist_ok=True)
    for item in buildpath.iterdir():
        pathlib.Path.unlink(item)
    if data[key_path]['target'] == 'exe':
        pathlib.Path.unlink(f"./dist/apps/{key}/launch/scripts/cpp-module.js", missing_ok=True)
        pathlib.Path.unlink(f"./dist/apps/{key}/launch/scripts/cpp-module.wasm", missing_ok=True)

    # get path of emcc
    emcc_path = shutil.which('emcc')

    # build all apps files into object files
    objs = []
    srcpath = pathlib.Path(f"./{key_path}/src")
    for item in srcpath.iterdir():
        obj = [f"./{key_path}/build/{item.name}.o"]
        objs += obj
        command = [emcc_path]
        command += flags
        command += objectflags
        command += [f"-I./{key_path}/include"]
        command += [f"-I./libs/{item}/include" for item in data[key_path]['dependencies']]
        command += [f"./{key_path}/src/{item.name}"]
        command += ['-o']
        command += obj
        print(command)
        subprocess.run(command) 

    # finally, build the target depending of if it is a lib or not
    if data[key_path]['target'] == 'lib':
        emar_path = shutil.which('emar')
        command = [emar_path]
        command += ['rcs']
        command += [f"./{key_path}/build/{key}.a"]
        command += objs
        print(command)
        subprocess.run(command) 
    else:
        command = [emcc_path]
        command += flags
        command += targetflags
        command += libs
        command += objs
        command += ['-o']
        command += [f"./dist/apps/{key}/launch/scripts/cpp-module.js"]
        print(command)
        subprocess.run(command) 

def run():  
    if len(sys.argv) > 1:
        if sys.argv[1] == "all":
            source_path = pathlib.Path(f"./source")
            for item in source_path.iterdir():
                build('source/' + item.name)
        else:
            build('source/' + sys.argv[1])
    else:
        print("Skipping builds.")


run()