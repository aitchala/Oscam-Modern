function addUnloadHandler() {
    var a, e;
    if (window.attachEvent) {
        a = window.attachEvent;
        e = 'on'
    } else {
        a = window.addEventListener;
        e = ''
    }
    a(e + 'load', function () {
        loadScroll();
        if (typeof Sys != 'undefined' && typeof Sys.WebForms != 'undefined') Sys.WebForms.PageRequestManager.getInstance().add_endRequest(loadScroll)
    }, false)
}

function addreader(){
    document.getElementById("newreader").style.display="block";
    document.getElementById("searchTable").style.display="none";cdpause();
    document.getElementById("timer").innerHTML='Refreshing has<br>been stopped!'
};


function adduser(){
    document.getElementById("newuser").style.display="block";
    document.getElementById("searchTable").style.display="none";cdpause();
    document.getElementById("timer").innerHTML='Refreshing has<br>been stopped!'
};

function startit() {
    if (document.all||document.getElementById)document.getElementById("timer").innerHTML='Next refresh<br>in <b id="countDownText" class="countDownText">'+countDownTime+' </b> seconds';
    countDown();
}

function cdpause() {
    clearTimeout(counter);
};

function loadScroll() {
    var c = document.cookie.split(';');
    for (var i = 0; i < c.length; i++) {
        var p = c[i].split('=');
        if (p[0] == 'scrollPosition') {
            p = unescape(p[1]).split('/');
            for (var j = 0; j < p.length; j++) {
                var e = p[j].split(',');
                try {
                    if (e[0] == 'window') {
                        window.scrollTo(e[1], e[2])
                    }
                } catch (ex) {}
            }
            return
        }
    }
}

function saveScroll() {
    var s = 'scrollPosition=';
    var l, t;
    if (window.pageXOffset !== undefined) {
        l = window.pageXOffset;
        t = window.pageYOffset
    } else if (document.documentElement && document.documentElement.scrollLeft !== undefined) {
        l = document.documentElement.scrollLeft;
        t = document.documentElement.scrollTop
    } else {
        l = document.body.scrollLeft;
        t = document.body.scrollTop
    } if (l || t) {
        s += 'window,' + l + ',' + t + '/'
    }
    document.cookie = s + ';'
}

function doSearch() {
    var searchText = document.getElementById('searchTerm').value;
    var targetTable = document.getElementById('dataTable');
    var targetTableColCount;
    for (var rowIndex = 0; rowIndex < targetTable.rows.length; rowIndex++) {
        var rowData = '';
        if (rowIndex == 0) {
            targetTableColCount = targetTable.rows.item(rowIndex).cells.length;
            continue
        }
        for (var colIndex = 0; colIndex < targetTableColCount; colIndex++) {
            var cellText = '';
            if (navigator.appName == 'Microsoft Internet Explorer') cellText = targetTable.rows.item(rowIndex).cells.item(colIndex).innerText;
            else cellText = targetTable.rows.item(rowIndex).cells.item(colIndex).textContent;
            rowData += cellText
        }
        rowData = rowData.toLowerCase();
        searchText = searchText.toLowerCase();
        if (rowData.indexOf(searchText) == -1) targetTable.rows.item(rowIndex).style.display = 'none';
        else targetTable.rows.item(rowIndex).style.display = 'table-row'
    }
}

(function (document) {
    'use strict';

    var LightTableSorter = (function (Arr) {

        var _th, _cellIndex, _order = '';

        function _text(row) {
            return row.cells.item(_cellIndex).textContent.toLowerCase();
        }

        function _sort(a, b) {
            var va = _text(a),
                vb = _text(b),
                n = parseInt(va, 10);
            if (n) {
                va = n;
                vb = parseInt(vb, 10);
            }
            return va > vb ? 1 : va < vb ? -1 : 0;
        }

        function _toggle() {
            var c = _order !== 'asc' ? 'asc' : 'desc';
            _th.className = (_th.className.replace(_order, '') + ' ' + c).trim();
            _order = c;
        }

        function _reset() {
            _th.className = _th.className.replace('asc', '').replace('desc', '');
            _order = '';
        }

        function onClickEvent(e) {
	    clearTimeout(counter);
            if (_th && _cellIndex !== e.target.cellIndex) {
                _reset();
            }
            _th = e.target;
            if (_th.nodeName.toLowerCase() === 'th') {
                _cellIndex = _th.cellIndex;
                var tbody = _th.offsetParent.getElementsByTagName('tbody')[0],
                    rows = tbody.rows;
                if (rows) {
                    rows = Arr.sort.call(Arr.slice.call(rows, 0), _sort);
                    if (_order === 'asc') {
                        Arr.reverse.call(rows);
                    }
                    _toggle();
                    tbody.innerHtml = '';
                    Arr.forEach.call(rows, function (row) {
                        tbody.appendChild(row);
                    });
                }
            }
        }

        return {
            init: function () {
                var ths = document.getElementsByTagName('th');
                Arr.forEach.call(ths, function (th) {
                    th.onclick = onClickEvent;
                });
            }
        };
    })(Array.prototype);

    document.addEventListener('readystatechange', function () {
        if (document.readyState === 'complete') {
            LightTableSorter.init();
        }
    });

})(document);