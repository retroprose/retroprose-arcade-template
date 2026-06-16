py ./scripts/build.py %1
start msedge.exe -inprivate http://localhost:8080/
py -m http.server 8080 -d ./dist