param(
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Release"
)

$ErrorActionPreference = "Stop"
$repositoryRoot = Split-Path -Parent $PSScriptRoot
$dependencyRoot = Join-Path $repositoryRoot "src\build\dependencies"
$solution = Join-Path $repositoryRoot "src\Twitch.sln"
$sdks = @("sdk2013", "bms", "portal2", "l4d", "l4d2")

New-Item -ItemType Directory -Force -Path $dependencyRoot | Out-Null

foreach ($sdk in $sdks) {
    $sdkRoot = Join-Path $dependencyRoot "hl2sdk-$sdk"
    if (-not (Test-Path (Join-Path $sdkRoot ".git"))) {
        git clone --depth 1 --branch $sdk https://github.com/alliedmodders/hl2sdk.git $sdkRoot
        if ($LASTEXITCODE -ne 0) { throw "Failed to clone the $sdk HL2SDK branch." }
    }

    msbuild $solution /m /t:Rebuild "/p:Configuration=$Configuration" /p:Platform=x86 "/p:SourceEngine=$sdk" "/p:HL2SDKRoot=$sdkRoot"
    if ($LASTEXITCODE -ne 0) { throw "The $sdk build failed." }
}
