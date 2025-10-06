let ws;

function initWebSocket() {
  ws = new WebSocket("ws://" + location.host + "/ws");

  ws.onopen = () => {
    document.getElementById("status").textContent = "🟢 Connecté au WebSocket";
  };

  ws.onclose = () => {
    document.getElementById("status").textContent = "🔴 Déconnecté. Reconnexion...";
    setTimeout(initWebSocket, 2000);
  };

  ws.onmessage = (event) => {
    console.log("WS >>", event.data);

    // Highlight cue déclenché
    if (event.data.startsWith("cueTrigger:")) {
      const index = parseInt(event.data.split(":")[1]);
      highlightCue(index);
    }
  };
}

function sendCue(index) {
  const input = document.getElementById("text" + index);
  const text = input.value.trim();
  if (ws && ws.readyState === WebSocket.OPEN) {
    ws.send(JSON.stringify({ cue: index, text: text }));
  }
}

function highlightCue(index) {
  const el = document.getElementById("cue" + index);
  el.classList.add("active");
  setTimeout(() => el.classList.remove("active"), 1000);
}

initWebSocket();
