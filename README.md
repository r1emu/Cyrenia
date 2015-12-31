# Cyrenia
Cyrenia is a plugginable network proxy packet logger. It works without being injected in the target process at all.

How to use :
 - Launch `Racia.py` ; Chose a target server which will be proxified.
 - Launch `Floana.exe` ; Chose to create a new barrack proxy with the target server IP/Port previously chosen, with optional plugins.
    - For instance : `./Floana.exe 125.141.153.219 2000 2000 barrack new ../Plugins/hook_zone_connect.dll`.
 - At this point, you can launch your client as usual.
 - At the login screen, chose "(Proxy)" server.
