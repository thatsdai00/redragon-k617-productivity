# Redragon K617 firmware tool - no install (Windows PowerShell + WinForms)
Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing
[System.Windows.Forms.Application]::EnableVisualStyles()

$Root = Split-Path -Parent $MyInvocation.MyCommand.Path
$Tool = Join-Path $Root "tools\sinowisp.exe"
$CustomHex = Join-Path $Root "firmware\redragon-k617.hex"
$StockHex = Join-Path $Root "firmware\stok-k617.hex"
$UserBackup = Join-Path $Root "firmware\benim-stok-yedek.hex"
$LangFile = Join-Path $Root "ui-lang.txt"
$Device = "redragon-k617-fizz"

# --- theme (dark) ---
$C = @{
  Bg       = [System.Drawing.Color]::FromArgb(18, 18, 20)
  Panel    = [System.Drawing.Color]::FromArgb(28, 28, 32)
  Card     = [System.Drawing.Color]::FromArgb(36, 36, 42)
  CardHot  = [System.Drawing.Color]::FromArgb(46, 46, 54)
  Border   = [System.Drawing.Color]::FromArgb(58, 58, 68)
  Text     = [System.Drawing.Color]::FromArgb(236, 236, 240)
  Muted    = [System.Drawing.Color]::FromArgb(140, 140, 150)
  Accent   = [System.Drawing.Color]::FromArgb(220, 38, 38)   # Redragon red
  Accent2  = [System.Drawing.Color]::FromArgb(255, 72, 72)
  Ok       = [System.Drawing.Color]::FromArgb(52, 199, 123)
  Warn     = [System.Drawing.Color]::FromArgb(255, 179, 64)
  LogBg    = [System.Drawing.Color]::FromArgb(14, 14, 16)
}

$Lang = "tr"
if (Test-Path -LiteralPath $LangFile) {
  $saved = (Get-Content -LiteralPath $LangFile -TotalCount 1 -ErrorAction SilentlyContinue)
  if ($saved -eq "en" -or $saved -eq "tr") { $Lang = $saved }
}

$i18n = @{
  tr = @{
    Title      = "Redragon K617"
    Sub        = "Kurulum yok. Klavyeyi USB ile tak. Firmware: {0}"
    BadgeOk    = "Yedek: ALINDI"
    BadgeNo    = "Yedek: YOK - once yedek al"
    Btn1       = "1  Kontrol paneli  (RGB / idle / Snap Tap)"
    Btn2       = "2  Orijinal firmwareyi yedekle  (onerilir)"
    Btn3       = "3  Ozel firmware kur"
    Btn4       = "4  Stok (fabrika) firmwareye geri don"
    Ready      = "Hazir. Klavyeyi USB ile tak."
    PanelOpen  = "Kontrol paneli tarayicida acildi."
    BackupRead = "Stok firmware okunuyor... Klavyeyi cikarma."
    BackupOk   = "Yedek kaydedildi: firmware\benim-stok-yedek.hex ({0} byte)"
    BackupMsg  = "Orijinal firmware yedegi alindi.`n`nDosya: firmware\benim-stok-yedek.hex"
    BackupFail = "Yedek dosyasi olusmadi."
    NoCustom   = "firmware\redragon-k617.hex bulunamadi."
    NoBackupQ  = "Orijinal (stok) firmware yedegi bulunamadi.`n`nLutfen once ""Orijinal firmwareyi yedekle"" ile yedek alin.`nYedek olmadan sorun olursa geri donmek zorlasabilir.`n`nYine de ozel firmware kurulumuna devam edilsin mi?"
    NoBackupT  = "Yedek yok - emin misin?"
    CancelBak  = "Kurulum iptal: once yedek alman onerilir."
    ConfirmC   = "Ozel firmware klavyeye yazilacak.`n`nDevam edilsin mi?"
    ConfirmT   = "Onay"
    CancelInst = "Kurulum iptal edildi."
    Installing = "Ozel firmware yukleniyor..."
    Installed  = "Ozel firmware kuruldu."
    InstMsg    = "Firmware yuklendi.`nKontrol paneli ile ayarlari yapabilirsin."
    Done       = "Tamam"
    Err        = "Hata"
    NoStock    = "Stok geri donmek icin once kendi yedegin lazim (benim-stok-yedek.hex)."
    StockQ     = "Stok firmware yazilacak:`n{0}`n`nDevam?"
    StockUser  = "senin yedegin (benim-stok-yedek.hex)"
    StockPkg   = "paket stok dosyasi yok"
    StockCancel= "Stok geri yukleme iptal."
    StockLoad  = "Stok yukleniyor ({0})..."
    StockOk    = "Stok firmware yuklendi."
    ToolMiss   = "tools\sinowisp.exe bulunamadi. Klasoru bozmadan kullan."
    ToolFail   = "Islem basarisiz (kod {0}). Klavyeyi USB ile takip tekrar dene."
    CmdPrefix  = "Komut: sinowisp "
  }
  en = @{
    Title      = "Redragon K617"
    Sub        = "No install. Plug in USB. Firmware: {0}"
    BadgeOk    = "Backup: DONE"
    BadgeNo    = "Backup: NONE - back up first"
    Btn1       = "1  Open control panel  (RGB / idle / Snap Tap)"
    Btn2       = "2  Backup original firmware  (recommended)"
    Btn3       = "3  Install custom firmware"
    Btn4       = "4  Restore stock (factory) firmware"
    Ready      = "Ready. Connect the keyboard via USB."
    PanelOpen  = "Control panel opened in browser."
    BackupRead = "Reading stock firmware... do not unplug."
    BackupOk   = "Backup saved: firmware\benim-stok-yedek.hex ({0} bytes)"
    BackupMsg  = "Original firmware backup saved.`n`nFile: firmware\benim-stok-yedek.hex"
    BackupFail = "Backup file was not created."
    NoCustom   = "firmware\redragon-k617.hex not found."
    NoBackupQ  = "No original (stock) firmware backup found.`n`nPlease run ""Backup original firmware"" first.`nWithout a backup, recovery is harder if something goes wrong.`n`nInstall custom firmware anyway?"
    NoBackupT  = "No backup - are you sure?"
    CancelBak  = "Install cancelled: back up first."
    ConfirmC   = "Custom firmware will be written to the keyboard.`n`nContinue?"
    ConfirmT   = "Confirm"
    CancelInst = "Install cancelled."
    Installing = "Installing custom firmware..."
    Installed  = "Custom firmware installed."
    InstMsg    = "Firmware installed.`nUse the control panel to change settings."
    Done       = "OK"
    Err        = "Error"
    NoStock    = "To restore stock you need your own backup first (benim-stok-yedek.hex)."
    StockQ     = "Stock firmware will be written:`n{0}`n`nContinue?"
    StockUser  = "your backup (benim-stok-yedek.hex)"
    StockPkg   = "no bundled stock file"
    StockCancel= "Stock restore cancelled."
    StockLoad  = "Loading stock ({0})..."
    StockOk    = "Stock firmware restored."
    ToolMiss   = "tools\sinowisp.exe missing. Keep the folder intact."
    ToolFail   = "Operation failed (code {0}). Replug USB and retry."
    CmdPrefix  = "Cmd: sinowisp "
  }
}

function T([string]$key) {
  return $i18n[$Lang][$key]
}

function Save-Lang {
  Set-Content -LiteralPath $LangFile -Value $Lang -Encoding ASCII
}

function Test-BackupExists {
  return (Test-Path -LiteralPath $UserBackup)
}

function Write-Log([string]$msg) {
  $stamp = Get-Date -Format "HH:mm:ss"
  $log.AppendText("[$stamp] $msg`r`n")
  $log.SelectionStart = $log.TextLength
  $log.ScrollToCaret()
  [System.Windows.Forms.Application]::DoEvents()
}

function Get-FwVer {
  $fwVer = "v53"
  $verFile = Join-Path $Root "firmware\VERSION.txt"
  if (Test-Path -LiteralPath $verFile) {
    $first = (Get-Content -LiteralPath $verFile -TotalCount 1 -ErrorAction SilentlyContinue)
    if ($first -match 'v(\d+)') { $fwVer = "v$($Matches[1])" }
  }
  return $fwVer
}

function Update-BackupBadge {
  if (Test-BackupExists) {
    $badge.Text = (T "BadgeOk")
    $badge.ForeColor = $C.Ok
  } else {
    $badge.Text = (T "BadgeNo")
    $badge.ForeColor = $C.Warn
  }
}

function Update-UiText {
  $form.Text = (T "Title")
  $title.Text = (T "Title")
  $sub.Text = ((T "Sub") -f (Get-FwVer))
  $btn1.Text = (T "Btn1")
  $btn2.Text = (T "Btn2")
  $btn3.Text = (T "Btn3")
  $btn4.Text = (T "Btn4")
  Update-BackupBadge
  if ($Lang -eq "tr") {
    $btnTr.BackColor = $C.Accent
    $btnTr.ForeColor = [System.Drawing.Color]::White
    $btnEn.BackColor = $C.Card
    $btnEn.ForeColor = $C.Muted
  } else {
    $btnEn.BackColor = $C.Accent
    $btnEn.ForeColor = [System.Drawing.Color]::White
    $btnTr.BackColor = $C.Card
    $btnTr.ForeColor = $C.Muted
  }
}

function Invoke-Tool([string[]]$ArgsList) {
  if (-not (Test-Path -LiteralPath $Tool)) {
    throw (T "ToolMiss")
  }
  Write-Log ((T "CmdPrefix") + ($ArgsList -join " "))
  $p = Start-Process -FilePath $Tool -ArgumentList $ArgsList -Wait -PassThru -NoNewWindow `
    -RedirectStandardOutput (Join-Path $env:TEMP "rdk617-out.txt") `
    -RedirectStandardError (Join-Path $env:TEMP "rdk617-err.txt")
  $out = ""
  if (Test-Path (Join-Path $env:TEMP "rdk617-out.txt")) {
    $out += Get-Content (Join-Path $env:TEMP "rdk617-out.txt") -Raw -ErrorAction SilentlyContinue
  }
  if (Test-Path (Join-Path $env:TEMP "rdk617-err.txt")) {
    $out += Get-Content (Join-Path $env:TEMP "rdk617-err.txt") -Raw -ErrorAction SilentlyContinue
  }
  if ($out) { Write-Log $out.Trim() }
  if ($p.ExitCode -ne 0) {
    throw ((T "ToolFail") -f $p.ExitCode)
  }
}

function New-MainButton([string]$text, [int]$y, [scriptblock]$click) {
  $b = New-Object System.Windows.Forms.Button
  $b.Text = $text
  $b.Location = New-Object System.Drawing.Point(28, $y)
  $b.Size = New-Object System.Drawing.Size(464, 52)
  $b.FlatStyle = "Flat"
  $b.BackColor = $C.Card
  $b.ForeColor = $C.Text
  $b.FlatAppearance.BorderColor = $C.Border
  $b.FlatAppearance.BorderSize = 1
  $b.FlatAppearance.MouseOverBackColor = $C.CardHot
  $b.FlatAppearance.MouseDownBackColor = $C.Panel
  $b.TextAlign = "MiddleLeft"
  $b.Padding = New-Object System.Windows.Forms.Padding(16, 0, 0, 0)
  $b.Font = New-Object System.Drawing.Font("Segoe UI Semibold", 10.5)
  $b.Cursor = [System.Windows.Forms.Cursors]::Hand
  $b.Add_Click($click)
  $form.Controls.Add($b)
  return $b
}

# --- form ---
$form = New-Object System.Windows.Forms.Form
$form.Text = "Redragon K617"
$form.Size = New-Object System.Drawing.Size(540, 620)
$form.StartPosition = "CenterScreen"
$form.FormBorderStyle = "FixedDialog"
$form.MaximizeBox = $false
$form.MinimizeBox = $true
$form.BackColor = $C.Bg
$form.Font = New-Object System.Drawing.Font("Segoe UI", 10)
$form.ForeColor = $C.Text

# top accent bar
$bar = New-Object System.Windows.Forms.Panel
$bar.Location = New-Object System.Drawing.Point(0, 0)
$bar.Size = New-Object System.Drawing.Size(540, 4)
$bar.BackColor = $C.Accent
$form.Controls.Add($bar)

$title = New-Object System.Windows.Forms.Label
$title.Text = "Redragon K617"
$title.Font = New-Object System.Drawing.Font("Segoe UI Semibold", 20)
$title.ForeColor = $C.Text
$title.Location = New-Object System.Drawing.Point(28, 22)
$title.AutoSize = $true
$form.Controls.Add($title)

# language toggle
$btnTr = New-Object System.Windows.Forms.Button
$btnTr.Text = "TR"
$btnTr.Location = New-Object System.Drawing.Point(400, 26)
$btnTr.Size = New-Object System.Drawing.Size(44, 28)
$btnTr.FlatStyle = "Flat"
$btnTr.FlatAppearance.BorderColor = $C.Border
$btnTr.FlatAppearance.BorderSize = 1
$btnTr.Font = New-Object System.Drawing.Font("Segoe UI Semibold", 9)
$btnTr.Cursor = [System.Windows.Forms.Cursors]::Hand
$btnTr.Add_Click({
  $script:Lang = "tr"
  Save-Lang
  Update-UiText
  Write-Log "Dil: Turkce"
})
$form.Controls.Add($btnTr)

$btnEn = New-Object System.Windows.Forms.Button
$btnEn.Text = "EN"
$btnEn.Location = New-Object System.Drawing.Point(448, 26)
$btnEn.Size = New-Object System.Drawing.Size(44, 28)
$btnEn.FlatStyle = "Flat"
$btnEn.FlatAppearance.BorderColor = $C.Border
$btnEn.FlatAppearance.BorderSize = 1
$btnEn.Font = New-Object System.Drawing.Font("Segoe UI Semibold", 9)
$btnEn.Cursor = [System.Windows.Forms.Cursors]::Hand
$btnEn.Add_Click({
  $script:Lang = "en"
  Save-Lang
  Update-UiText
  Write-Log "Language: English"
})
$form.Controls.Add($btnEn)

$sub = New-Object System.Windows.Forms.Label
$sub.ForeColor = $C.Muted
$sub.Location = New-Object System.Drawing.Point(30, 60)
$sub.Size = New-Object System.Drawing.Size(480, 22)
$sub.Font = New-Object System.Drawing.Font("Segoe UI", 9.5)
$form.Controls.Add($sub)

$badge = New-Object System.Windows.Forms.Label
$badge.Location = New-Object System.Drawing.Point(30, 88)
$badge.Size = New-Object System.Drawing.Size(480, 22)
$badge.Font = New-Object System.Drawing.Font("Segoe UI Semibold", 9.5)
$form.Controls.Add($badge)

$btn1 = New-MainButton "" 126 {
  Start-Process "https://thatsdai.pages.dev"
  Write-Log (T "PanelOpen")
}

$btn2 = New-MainButton "" 188 {
  try {
    $form.UseWaitCursor = $true
    Write-Log (T "BackupRead")
    $fwDir = Join-Path $Root "firmware"
    if (-not (Test-Path $fwDir)) { New-Item -ItemType Directory -Path $fwDir | Out-Null }
    Invoke-Tool @("read", "-d", $Device, "-s", "firmware", "--reboot", "true", $UserBackup)
    if (-not (Test-Path $UserBackup)) { throw (T "BackupFail") }
    $len = (Get-Item $UserBackup).Length
    Write-Log ((T "BackupOk") -f $len)
    Update-BackupBadge
    [System.Windows.Forms.MessageBox]::Show((T "BackupMsg"), (T "Title"), "OK", "Information") | Out-Null
  } catch {
    Write-Log ("HATA / ERROR: " + $_.Exception.Message)
    [System.Windows.Forms.MessageBox]::Show($_.Exception.Message, (T "Err"), "OK", "Error") | Out-Null
  } finally {
    $form.UseWaitCursor = $false
  }
}

$btn3 = New-MainButton "" 250 {
  try {
    if (-not (Test-Path $CustomHex)) { throw (T "NoCustom") }

    if (-not (Test-BackupExists)) {
      $ans = [System.Windows.Forms.MessageBox]::Show((T "NoBackupQ"), (T "NoBackupT"), "YesNo", "Warning")
      if ($ans -ne [System.Windows.Forms.DialogResult]::Yes) {
        Write-Log (T "CancelBak")
        return
      }
    } else {
      $ans = [System.Windows.Forms.MessageBox]::Show((T "ConfirmC"), (T "ConfirmT"), "YesNo", "Question")
      if ($ans -ne [System.Windows.Forms.DialogResult]::Yes) {
        Write-Log (T "CancelInst")
        return
      }
    }

    $form.UseWaitCursor = $true
    Write-Log (T "Installing")
    Invoke-Tool @("write", "-d", $Device, "--force", "--reboot", "true", $CustomHex)
    Write-Log (T "Installed")
    [System.Windows.Forms.MessageBox]::Show((T "InstMsg"), (T "Done"), "OK", "Information") | Out-Null
  } catch {
    Write-Log ("HATA / ERROR: " + $_.Exception.Message)
    [System.Windows.Forms.MessageBox]::Show($_.Exception.Message, (T "Err"), "OK", "Error") | Out-Null
  } finally {
    $form.UseWaitCursor = $false
  }
}

$btn4 = New-MainButton "" 312 {
  try {
    $src = $null
    $label = ""
    if (Test-BackupExists) {
      $src = $UserBackup
      $label = (T "StockUser")
    } elseif (Test-Path $StockHex) {
      $src = $StockHex
      $label = (T "StockPkg")
    } else {
      throw (T "NoStock")
    }

    $ans = [System.Windows.Forms.MessageBox]::Show(((T "StockQ") -f $label), (T "ConfirmT"), "YesNo", "Question")
    if ($ans -ne [System.Windows.Forms.DialogResult]::Yes) {
      Write-Log (T "StockCancel")
      return
    }

    $form.UseWaitCursor = $true
    Write-Log ((T "StockLoad") -f $label)
    Invoke-Tool @("write", "-d", $Device, "--force", "--reboot", "true", $src)
    Write-Log (T "StockOk")
    [System.Windows.Forms.MessageBox]::Show((T "StockOk"), (T "Done"), "OK", "Information") | Out-Null
  } catch {
    Write-Log ("HATA / ERROR: " + $_.Exception.Message)
    [System.Windows.Forms.MessageBox]::Show($_.Exception.Message, (T "Err"), "OK", "Error") | Out-Null
  } finally {
    $form.UseWaitCursor = $false
  }
}

$log = New-Object System.Windows.Forms.TextBox
$log.Multiline = $true
$log.ReadOnly = $true
$log.ScrollBars = "Vertical"
$log.Location = New-Object System.Drawing.Point(28, 384)
$log.Size = New-Object System.Drawing.Size(464, 168)
$log.BackColor = $C.LogBg
$log.ForeColor = $C.Muted
$log.BorderStyle = "FixedSingle"
$log.Font = New-Object System.Drawing.Font("Consolas", 9)
$form.Controls.Add($log)

Update-UiText
Write-Log (T "Ready")
[void]$form.ShowDialog()
