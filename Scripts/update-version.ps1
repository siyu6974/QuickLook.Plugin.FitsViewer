﻿$tag = git describe --always --tags "--abbrev=0"
Write-Host "Tag: $tag"

$revision = git describe --always --tags
Write-Host "Revision: $revision"

$text = @"
// This file is generated by update-version.ps1

using System.Reflection;

[assembly: AssemblyVersion("$tag")]
[assembly: AssemblyInformationalVersion("$revision")]
"@

Write-Host "C# Code Block: `n$text"
$filePath = "$PSScriptRoot\..\GitVersion.cs"
Write-Host "Writing to file: $filePath"
$text | Out-File $filePath -Encoding utf8

$xml = [xml](Get-Content $PSScriptRoot\..\QuickLook.Plugin.Metadata.Base.config)
$xmlPath = "$PSScriptRoot\..\QuickLook.Plugin.Metadata.Base.config"
Write-Host "Reading XML from: $xmlPath"
$xml = [xml](Get-Content $xmlPath)

$xml.Metadata.Version = "$revision"
Write-Host "Updated XML: `n$xml.OuterXml"

$savePath = "$PSScriptRoot\..\QuickLook.Plugin.Metadata.config"
Write-Host "Saving XML to: $savePath"
$xml.Save($savePath)
