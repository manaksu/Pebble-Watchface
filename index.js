/* PebbleKit JS — Literary Watchface */

function getSettings() {
  return {
    w: localStorage.getItem('widget') || '3',
    f: localStorage.getItem('font')   || '1',
    s: localStorage.getItem('style')  || '0',
    c: localStorage.getItem('colour') || '1'
  };
}

function send(cfg) {
  Pebble.sendAppMessage({
    0: parseInt(cfg.w, 10),
    1: parseInt(cfg.f, 10),
    2: parseInt(cfg.s, 10),
    3: parseInt(cfg.c, 10)
  }, function(){}, function(){});
}

function buildURL() {
  var cfg = getSettings();

  function group(name, label, opts, sel) {
    return '<p><b>' + label + '</b></p>' +
      opts.map(function(l, i) {
        return '<p><label><input type=radio name=' + name + ' value=' + i +
               (String(i) === sel ? ' checked' : '') + '> ' + l + '</label></p>';
      }).join('');
  }

  /* Build the close URL upfront with default values */
  var closeBase = 'pebblejs://close#';

  var html = '<!DOCTYPE html><html><head>'
    + '<meta name=viewport content="width=device-width,initial-scale=1">'
    + '<style>'
    + 'body{font-family:sans-serif;padding:12px;font-size:16px;margin:0}'
    + 'b{display:block;margin-top:14px;font-size:12px;color:#888;text-transform:uppercase;font-weight:normal}'
    + 'p{margin:4px 0}'
    + 'a#btn{display:block;margin-top:20px;padding:14px;background:#e02020;'
    + 'color:#fff;text-align:center;text-decoration:none;border-radius:8px;font-size:16px}'
    + '</style></head><body>'
    + group('w', 'Widget',     ['Sine Wave','Bar Chart','Battery Line','Dot ECG'],  cfg.w)
    + group('f', 'Font',       ['Compact','Standard','Bold','Large'],                cfg.f)
    + group('s', 'Text Style', ['ALL CAPS','Mixed Case','all lower','BRIGHT UPPER'], cfg.s)
    + group('c', 'Colour',     ['Pure White','Warm White','Soft Amber','Cool Gray','Pale Green'], cfg.c)
    + '<a id=btn href=#>Save</a>'
    + '<script>'
    + 'function upd(){'
    + 'function v(n){return document.querySelector("input[name="+n+"]:checked").value;}'
    + 'document.getElementById("btn").href="pebblejs://close#"'
    + '+encodeURIComponent(JSON.stringify({widget:+v("w"),font:+v("f"),style:+v("s"),colour:+v("c")}));}'
    + 'document.querySelectorAll("input").forEach(function(i){i.onchange=upd;});'
    + 'upd();'
    + '<\/script>'
    + '</body></html>';

  return 'data:text/html,' + encodeURIComponent(html);
}

Pebble.addEventListener('ready', function() {
  send(getSettings());
});

Pebble.addEventListener('showConfiguration', function() {
  Pebble.openURL(buildURL());
});

Pebble.addEventListener('webviewclosed', function(e) {
  if (!e || !e.response || e.response === '' || e.response === 'CANCELLED') return;
  var raw = e.response;
  if (raw.indexOf('#') !== -1) raw = raw.split('#').pop();
  var cfg;
  try { cfg = JSON.parse(decodeURIComponent(raw)); }
  catch(err) { return; }
  if (cfg.widget  != null) localStorage.setItem('widget',  String(cfg.widget));
  if (cfg.font    != null) localStorage.setItem('font',    String(cfg.font));
  if (cfg.style   != null) localStorage.setItem('style',   String(cfg.style));
  if (cfg.colour  != null) localStorage.setItem('colour',  String(cfg.colour));
  send(getSettings());
});
