#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <crypt.h>

static
void
login_page(const char *msg)
{
	printf("Content-Type: text/html\n\n");

	puts(
	"<!DOCTYPE html>\n"
	"<html>\n"
	"  <head>\n"
	"    <title>portmanteau - login</title>\n"
	"  </head>\n"
	"  <body background=\"img/bg.jpg\">\n"
	"    <big>\n"
	"      <table>\n"
	"        <tbody>\n"
	"          <tr>\n"
	"            <td><a href=\"index.html\">home</a></td>\n"
	"            <td width=\"90%\"></td>\n"
	"            <td><a href=\"catalog.py\">catalog</a></td>\n"
	"            <td><a href=\"register.pl\">register</a></td>\n"
	"          </tr>\n"
	"        </tbody>\n"
	"      </table>\n"
	"    </big>\n"
	"    <center>\n"
	"      <h1>login</h1>\n"
	);

	if (msg) printf("<h4>%s</h4>\n", msg);

	puts(
	"      <form action=\"login.cgi\" method=\"POST\">\n"
	"        <label for=\"username\">Username</label>\n"
	"        <input type=\"text\" name=\"username\" /><br /><br />\n"
	"        <label for=\"password\">Password</label>\n"
	"        <input type=\"password\" name=\"password\" /><br /><br />\n"
	"        <input type=\"submit\" value=\"Login\" />\n"
	"      </form>\n"
	"    </center>\n"
	"  </body>\n"
	"</html>\n"
	);

	exit(0);
}

static
void
catalog_page(const char *user, const char *cookie)
{
	char cmd[0x100];
	const char *p = user;

	while (*p) if (!isalnum(*p++))
		login_page("Invalid character in username");

	if (cookie) {
		printf("Set-Cookie: session=%s\n", cookie);
		fflush(stdout);
	}

	snprintf(cmd, sizeof(cmd), "./catalog.py '%s'", user);
	exit(system(cmd));
}

static inline
void
chomp(char *str)
{
	if ((str = strrchr(str, '\n'))) *str = 0;
}

static
int
pwd_cmp(const char *plain, const char *hash)
{
	return plain && hash && !strcmp(crypt(plain, hash), hash);
}

static
int
generate_cookie(char *buffer, size_t size)
{
	FILE *urand;
	unsigned char raw[0x10];
	size_t i, r;

	if (!(urand = fopen("/dev/urandom", "rb"))) return -1;

	r = fread(raw, sizeof(raw), 1, urand);
	fclose(urand);

	if (!r) return -1;

	for (i = 0; i != sizeof(raw) && i != size / 2; ++i)
		sprintf(buffer + i * 2, "%02X", raw[i]);

	return 0;
}

static
int
parse_request(char *request, char *user, size_t usize,
	char *pwd, size_t psize)
{
	char *value, *key;

	if (!usize || !psize) return -1;

	for (key = strtok(request, "&"); key; key = strtok(NULL, "&")) {
		if (!(value = strchr(key, '='))) continue;
		*value++ = 0;

		if (!strcmp(key, "password"))
			strncpy(pwd, value, psize);

		else if (!strcmp(key, "username"))
			strncpy(user, value, usize);
	}

	return !*user || !*pwd;
}

static
int
match_user(const char *path, const char *user, const char *pwd)
{
	FILE *db;
	char buffer[0x1000];
	char *hash = NULL;

	if (!(db = fopen(path, "rb"))) return -1;

	while (fgets(buffer, sizeof(buffer), db)) {
		chomp(buffer);

		(void)(strtok(buffer, ",")); /* full name */

		if (strcmp(user, strtok(NULL, ","))) continue;

		hash = strtok(NULL, ",");
		break;
	}

	fclose(db);
	return hash && pwd_cmp(pwd, hash);
}

static
int
set_cookie(const char *path, const char *user, char *cookie, size_t size)
{
	FILE *db;
	char buffer[0x1000];
	char *old = NULL;

	if (!(db = fopen(path, "r+b"))) return -1;

	while (fgets(buffer, sizeof(buffer), db)) {
		chomp(buffer);

		if (strcmp(user, strtok(buffer, ","))) continue;

		old = strtok(NULL, ",");
		break;
	}

	if (old && *old) {
		strncpy(cookie, old, size);

	} else {
		generate_cookie(cookie, size);

		fseek(db, 0, SEEK_END);
		fprintf(db, "%s,%s\n", user, cookie);
	}

	fclose(db);
	return 0;
}

int
main(int argc, char **argv)
{
	char buffer[0x1000] = { 0 };
	char user[0x100], pwd[0x100], cookie[0x100] = { 0 };

	(void)(argc);
	(void)(argv);

	if (
		!getenv("REQUEST_METHOD") ||
		strcmp(getenv("REQUEST_METHOD"), "POST") ||
		!fread(buffer, 1, sizeof(buffer) - 1, stdin)
	)
		login_page(NULL);

	chomp(buffer);

	if (parse_request(buffer, user, sizeof(user), pwd, sizeof(pwd)))
		login_page("Missing username or password");

	if (!match_user("db/members.csv", user, pwd))
		login_page("Incorrect username or password");

	if (set_cookie("db/loggedin.csv", user, cookie, sizeof(cookie)))
		login_page("Internal error, please try again");

	catalog_page(user, cookie);
	return 0;
}
