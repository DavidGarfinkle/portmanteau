#!/usr/bin/python
from __future__ import print_function
import fcntl, cgi, csv, sys;

class open_locked:
    def __init__(self, *args, **kwargs):
        self.fd = open(*args, **kwargs)

    def __enter__(self):
        fcntl.flock(self.fd, fcntl.LOCK_EX)
        return self.fd.__enter__()

    def __exit__(self, type, value, traceback):
        fcntl.flock(self.fd, fcntl.LOCK_UN)
        return self.fd.__exit__()

inventory = []
with open_locked('db/inventory.csv') as db:
    inventory = [row for row in csv.reader(db)]

print("""\
Content-Type: text/html

<!DOCTYPE html>
<html>
  <head>
    <title>portmanteau - catalog</title>
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
      <h1>catalog</h1>
      <form action="purchase.py" method="POST">
        <table>
          <thead>
            <tr>
              <th>Article</th>
              <th></th>
              <th>Price</th>
              <th>Stock</th>
              <th></th>
              <th>Quantity to order</th>
            </tr>
          </thead>
          <tbody>
""")

for item in inventory:
    if int(item[5]):
        print("""
        <tr>
          <td width="500px"><b>{1}</b><br />{2}</td>
          <td><img src="{3}" width="100px" height="100px" /></td>
          <td>{4}$</td>
          <td>{5}</td>
          <td><input type="checkbox" name="{0}" /></td>
          <td><input type="number" name="{0}-count" value="0" /></td>
        </tr>
        """.format(*[cgi.escape(e) for e in item]))

print("""
          </tbody>
        </table>
        <input type="hidden" name="username" value="{0}"/>
        <input type="submit" value="Order" />
      </form>
    </center>
  </body>
</html>
""".format(sys.argv[1] if len(sys.argv) > 1 else ''))
