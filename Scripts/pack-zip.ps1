Remove-Item ..\QuickLook.Plugin.FitsViewer.qlplugin -ErrorAction SilentlyContinue

$files = Get-ChildItem -Path ..\Release\ -Exclude *.pdb,*.xml
Compress-Archive $files ..\QuickLook.Plugin.FitsViewer.zip
# Move-Item ..\QuickLook.Plugin.FitsViewer.zip ..\QuickLook.Plugin.FitsViewer.qlplugin