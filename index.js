/* PebbleKit JS — Literary Watchface */

var CONFIG_URL = 'https://manaksu.github.io/Pebble-Watchface/config.html';

function send(w, f, s, c) {
  Pebble.sendAppMessage(
    { 'widget': w, 'font': f, 'style': s, 'colour': c },
    function() { console.log('sent w='+w+' f='+f+' s='+s+' c='+c); },
    function(e) { console.log('fail: '+JSON.stringify(e)); }
  );
}

function getSettings() {
  return {
    w: parseInt(localStorage.getItem('widget') || '3', 10),
    f: parseInt(localStorage.getItem('font')   || '1', 10),
    s: parseInt(localStorage.getItem('style')  || '0', 10),
    c: parseInt(localStorage.getItem('colour') || '1', 10)
  };
}

Pebble.addEventListener('ready', function() {
  console.log('Literary JS ready');
  var cfg = getSettings();
  send(cfg.w, cfg.f, cfg.s, cfg.c);
});

Pebble.addEventListener('showConfiguration', function() {
  var cfg = getSettings();
  var url = CONFIG_URL
    + '?w=' + cfg.w
    + '&f=' + cfg.f
    + '&s=' + cfg.s
    + '&c=' + cfg.c;
  Pebble.openURL(url);
});

Pebble.addEventListener('webviewclosed', function(e) {
  if (!e || !e.response || e.response === '' || e.response === 'CANCELLED') return;
  var raw = e.response;
  if (raw.charAt(0) === '#') raw = raw.slice(1);
  var cfg;
  try { cfg = JSON.parse(decodeURIComponent(raw)); }
  catch(err) { console.log('parse error: ' + err); return; }
  console.log('config received: ' + JSON.stringify(cfg));
  if (cfg.widget  !== undefined) localStorage.setItem('widget',  String(cfg.widget));
  if (cfg.font    !== undefined) localStorage.setItem('font',    String(cfg.font));
  if (cfg.style   !== undefined) localStorage.setItem('style',   String(cfg.style));
  if (cfg.colour  !== undefined) localStorage.setItem('colour',  String(cfg.colour));
  var s = getSettings();
  send(s.w, s.f, s.s, s.c);
});
