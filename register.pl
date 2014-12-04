#!/usr/bin/perl
use strict;
use warnings;

use CGI;
use Fcntl qw/:flock SEEK_SET SEEK_END/;

sub lock { flock(shift, LOCK_EX) or die "cant lock - $!\n"; }
sub unlock { flock(shift, LOCK_UN) or die "cant unlock - $!\n"; }

sub rand_salt {
	return join '', ('.', '/', 0 .. 9, 'A' .. 'Z', 'a' .. 'z')
	[ map { rand 64 } 1 .. 16 ];
}

sub find_member {
	my ($db, $key) = @_;
	seek($db, 0, SEEK_SET);

	my $match = grep { $_->[1] eq $key } map { [ split /,/ ] } <$db>;

	seek($db, 0, SEEK_SET);
	return $match;
}

sub add_member {
	my $db = shift;
	seek($db, 0, SEEK_END);

	print $db (join(',', @_) . "\n");

	seek($db, 0, SEEK_SET);
}

sub parse_query {
	my $cgi = shift;
	$cgi->import_names();

	return undef unless (
		$Q::fullname &&
		$Q::username &&
		$Q::password &&
		$Q::cpassword
	);

	die "Invalid full name\n" unless $Q::fullname && $Q::fullname !~ /,/;

	die "Invalid username\n" unless $Q::username =~ /^[a-zA-Z0-9]+$/;

	die "Invalid password\n" unless $Q::password;

	die "Password mismatch\n" unless $Q::password eq $Q::cpassword;

	return {
		'fullname' => $Q::fullname,
		'username' => $Q::username,
		'password' => $Q::password,
	};
}

my $cgi = CGI->new;
my ($db, $m);

eval {
	return unless $m = parse_query($cgi);

	open($db, '+<', 'db/members.csv');
	lock($db);

	die "Username already exists\n" if (find_member($db, $m->{'username'}));

	add_member($db, $m->{'fullname'}, $m->{'username'}, crypt(
		$m->{'password'}, '$6$' . rand_salt() . '$'
	));
};

if ($db) {
	unlock($db);
	close($db);
};

print $cgi->header('text/html');

print <<END;
<!DOCTYPE html>
<html>
  <head>
    <title>portmanteau - register</title>
  </head>
  <body background="img/bg.jpg">
    <big>
      <table>
        <tbody>
          <tr>
            <td><a href="index.html">home</a></td>
            <td width="90%"></td>
            <td><a href="catalog.py">catalog</a></td>
            <td><a href="login.cgi">login</a></td>
          </tr>
        </tbody>
      </table>
    </big>
END

if (!$m || $@) {
	print <<END;
	<center>
	  <h1>register</h1>
END
	print '<h3>' . $cgi->escapeHTML($@) . '</h3>' if $@;
	print <<END;
	  <form action="register.pl" method="POST">
	    <label for="fullname">Full name</label>
	    <input type="text" name="fullname" /><br /><br />
	    <label for="username">Username</label>
	    <input type="text" name="username" /><br /><br />
	    <label for="password">Password</label>
	    <input type="password" name="password" /><br /><br />
	    <label for="cpassword">Password confirmation</label>
	    <input type="password" name="cpassword" /><br /><br />
	    <input type="submit" value="Register" />
	  </form>
	</center>
END

} else {
	print <<END;
	<center>
	  <h1>registration complete</h1>
	  <h2>Your account was successfully created,</h2>
	  <h2>you can now login and start shopping!</h2>
	</center>
END
}

print <<END;
  </body>
</html>
END
