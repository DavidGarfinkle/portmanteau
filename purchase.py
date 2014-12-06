#!/usr/bin/python
from __future__ import print_function
import Cookie, os, fcntl, cgi, csv

def lock(fd):
    fcntl.flock(fd, fcntl.LOCK_EX)

def unlock(fd):
    fcntl.flock(fd, fcntl.LOCK_UN)

class open_locked:
    def __init__(self, *args, **kwargs):
        self.fd = open(*args, **kwargs)

    def __enter__(self):
        lock(self.fd)
        return self.fd.__enter__()

    def __exit__(self, type, value, traceback):
        unlock(self.fd)
        return self.fd.__exit__()

def rchop(string, trail):
    length = len(trail)
    return string[:-length] if string[-length:] == trail else string

try:
    sessions = {}
    with open_locked('db/loggedin.csv') as fh:
        sessions = dict(((row[1], row) for row in csv.reader(fh)))

    cookie = Cookie.SimpleCookie(os.environ['HTTP_COOKIE'])
    user = sessions[cookie['session'].value]
except (Cookie.CookieError, KeyError):
    user = None

inv_db = open('db/inventory.csv', 'r+')
lock(inv_db)

inv_db.seek(0)
inventory = dict((
    (row[0], row)
    for row in csv.reader(inv_db)
    if row and len(row) > 5
))

form = cgi.FieldStorage()
order = dict((
    (key, int(form.getvalue(key + '-count')))
    for key in inventory if (
        key in form and key + '-count' in form and
        form.getvalue(key) and
        int(form.getvalue(key + '-count'))
    )
))

purchased = dict((
    (key, min(count, int(inventory[key][5])))
    for key, count in order.iteritems()
    if key in inventory and int(inventory[key][5])
))

print("""\
Content-Type: text/html

<!DOCTYPE html>
<html>
  <head>
    <title>portmanteau - bill</title>
  </head>
  <body background="img/bg.jpg">
    <big>
      <table>
        <tbody>
          <tr>
            <td><a href="index.html">home</a></td>
            <td width="100%"></td>
            <td><a href="catalog.py">catalog</a></td>
            <td><a href="login.cgi">login</a></td>
          </tr>
        </tbody>
      </table>
    </big>
    <center>
      <h1>bill</h1>
""")

if user is None:
    print("""
    <h2>
      Please <a href="login.cgi">log in</a> before placing an order.
    </h2>
    """)

elif not len(order):
    print("""
    <h2>
      Your order is empty. Please select the items you wish to purchase on
      the <a href="catalog.py">catalog</a> before placing your order.
    </h2>
    """)

elif not len(purchased):
    print("""
    <h2>
      None of the items you ordered are currently in stock,
      please order again later.
    </h2>
    """)

else:
    print("""
    <table>
      <thead>
        <tr>
          <th>Article</th>
          <th>Price</th>
          <th>Quantity</th>
          <th>Subtotal</th>
        </tr>
      </thead>
      <tbody>
    """)

    items = [[
        inventory[key][1],
        int(inventory[key][4]),
        purchased[key],
        purchased[key] * int(inventory[key][4])
    ] if key in purchased else [
        inventory[key][1],
        int(inventory[key][4]),
        'out of stock',
        '-'
    ] for key in order]

    for item in items:
        print("""
        <tr>
          <td>{0}</td>
          <td>{1}$</td>
          <td>{2}</td>
          <td>{3}$</td>
        </tr>
        """.format(*[cgi.escape(str(e)) for e in item]))

    print("""
      </tbody>
      <tfoot>
        <td><b>Total</b></td>
        <td></td>
        <td></td>
        <td>{0}$</td>
      </tfoot>
    </table>
    """.format(sum((
        item[3] for item in items
        if item[3] != '-'
    ))))

print("""
    </center>
  </body>
</html>
""")

for item in purchased:
    quantity = int(inventory[item][5])
    inventory[item][5] = str(quantity - purchased[item])

inv_db.seek(0)
csv.writer(inv_db).writerows(inventory.values())

unlock(inv_db)
inv_db.close()
