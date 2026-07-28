param(
    [Parameter(Position = 0)]
    [string]$Command = "echo Source Command Pipe is working",

    [ValidateRange(1, 30000)]
    [int]$TimeoutMilliseconds = 5000
)

$ErrorActionPreference = "Stop"
$pipe = [System.IO.Pipes.NamedPipeClientStream]::new(
    ".",
    "SourceCommands",
    [System.IO.Pipes.PipeDirection]::Out
)

try {
    $pipe.Connect($TimeoutMilliseconds)
    $encoding = [System.Text.UTF8Encoding]::new($false)
    $writer = [System.IO.StreamWriter]::new($pipe, $encoding)
    $writer.AutoFlush = $true
    $writer.Write($Command)
    Write-Host "Sent command: $Command"
}
finally {
    if ($null -ne $writer) { $writer.Dispose() }
    $pipe.Dispose()
}
