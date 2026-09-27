#pragma once

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Quadruped Controller</title>
  <style>
    body { margin: 0; min-height: 100vh; display: grid; place-items: center;
      background: #202020; color: #fff; font-family: system-ui, sans-serif; text-align: center; }
    main { padding: 20px; }
    h1 { font-size: 24px; }
    #status { color: #bbb; }
    .pad { display: grid; grid-template-columns: repeat(3, minmax(0, 100px));
      grid-template-rows: repeat(3, 90px); gap: 10px; margin: 28px auto; }
    button { border: 2px solid #666; border-radius: 16px; background: #333;
      color: white; font: inherit; cursor: pointer; touch-action: manipulation; }
    button[aria-pressed="true"] { background: #176a50; border-color: #70efbb; }
    button:disabled { opacity: .4; cursor: default; }
    button:focus-visible { outline: 3px solid white; outline-offset: 3px; }
    #forward { grid-area: 1 / 2; } #backward { grid-area: 3 / 2; }
    #left { grid-area: 2 / 1; } #right { grid-area: 2 / 3; }
    #stop { grid-area: 2 / 2; border-radius: 50%; width: 80px; height: 80px; place-self: center; }
    .hint { max-width: 320px; color: #bbb; font-size: 14px; }
  </style>
</head>
<body>
<main>
  <h1>Quadruped Controller</h1>
  <p id="status" role="status">Disconnected</p>
  <div class="pad" aria-label="Direction controls">
    <button id="forward" aria-pressed="false" disabled>↑<br>Forward</button>
    <button id="left" aria-pressed="false" disabled>←<br>Left</button>
    <button id="stop" aria-label="Stop all directions" aria-pressed="true" disabled>●</button>
    <button id="right" aria-pressed="false" disabled>→<br>Right</button>
    <button id="backward" aria-pressed="false" disabled>↓<br>Backward</button>
  </div>
  <p id="selection" aria-live="polite">Stopped</p>
  <p class="hint">Tap a direction to toggle it. Combine forward or backward with left or right. Tap the circle to clear all directions.</p>
</main>
<script>
let websocket;
let longitudinal = '';
let lateral = '';
const buttons = [...document.querySelectorAll('button')];

function render() {
  for (const direction of ['forward', 'backward', 'left', 'right']) {
    document.getElementById(direction).setAttribute('aria-pressed',
      String(longitudinal === direction || lateral === direction));
  }
  document.getElementById('stop').setAttribute('aria-pressed', String(!longitudinal && !lateral));
  document.getElementById('selection').textContent =
    [longitudinal, lateral].filter(Boolean).join(' + ') || 'Stopped';
}

function sendState() {
  if (!websocket || websocket.readyState !== WebSocket.OPEN) return;
  websocket.send([longitudinal, lateral].filter(Boolean).join('_').toUpperCase() || 'STOP');
}

function stop() {
  longitudinal = lateral = '';
  render();
  sendState();
}

for (const direction of ['forward', 'backward', 'left', 'right']) {
  document.getElementById(direction).addEventListener('click', () => {
    if (direction === 'forward' || direction === 'backward') {
      longitudinal = longitudinal === direction ? '' : direction;
    } else {
      lateral = lateral === direction ? '' : direction;
    }
    render();
    sendState();
  });
}
document.getElementById('stop').addEventListener('click', stop);
window.addEventListener('pagehide', stop);
document.addEventListener('visibilitychange', () => { if (document.hidden) stop(); });

function initWebSocket() {
  websocket = new WebSocket('ws://' + window.location.host + '/ws');
  websocket.onopen = () => {
    document.getElementById('status').textContent = 'Connected';
    buttons.forEach(button => button.disabled = false);
    stop();
  };
  websocket.onmessage = event => {
    const parts = event.data.toLowerCase().split('_');
    longitudinal = parts.find(part => part === 'forward' || part === 'backward') || '';
    lateral = parts.find(part => part === 'left' || part === 'right') || '';
    render();
  };
  websocket.onclose = () => {
    document.getElementById('status').textContent = 'Disconnected — reconnecting…';
    buttons.forEach(button => button.disabled = true);
    longitudinal = lateral = '';
    render();
    setTimeout(initWebSocket, 1000);
  };
  websocket.onerror = () => websocket.close();
}
initWebSocket();
</script>
</body>
</html>
)rawliteral";
