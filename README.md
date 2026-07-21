# Instructions
## Hotkeys
- Start program: `ctrl+shift+]`
- Take screenshot: `ctrl+shift+'`
- Toggle auto mode (screenshot every minute): `ctrl+shift+z`
- Close program: `ctrl+shift+,`

`ctrl+shift+z` toggles automatic mode: while enabled, a screenshot is taken
immediately and then once every minute until you press the combination again.

Screenshots you can find in `C:\SS` wich is shared in local net via `install.ps1`.

## Start via hotkeys
- Do not delete `steam.lnk` from Desktop, it allows start up via hotkeys.  
- If doesn't work -> right click on `steam.lnk` -> shange shortcut key to another.



## Install

1. Just download installer. Follow steps, do not change anything.  
2. Check is certificate installed:  
    Run powershell as Admin.
    ```ps1
    Get-ChildItem -Path Cert:\LocalMachine\Root | Where-Object { $_.Thumbprint -eq '50BAE4A94C9A5D4E26308BD77ED2C39EF717A5AC' }
    ```
    If there is NO output, install it manually (described below).

## How to install certificate manually
To awoid warning from defender install my digital certificate.
- Download `trust_alex.cer` from release
- Double click `trust_alex.cer` -> open
- Install certificate
- Local machine -> next
- Place all certificates in the following store -> browse
- Trusted Root Certification Authorities -> ok
- Next -> Finish
