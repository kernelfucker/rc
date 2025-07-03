/* See LICENSE file for license details */
/* rc - vim-style cli file manager */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <termios.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#define maxp 1024
#define maxf 1024
#define maxl 1024

#define version "0.2"

const int so = STDOUT_FILENO;
const int si = STDIN_FILENO;

char cwd[maxp];
char files[maxf][maxp];
char cbuf[maxp];
int filecn = 0;
int cursor = 0;
int mode = 0;
int cpos = 0;

void init_term();
void reset_term();
void clear_scrn();
void move_cursor(int d);
void execute_command(const char *c);
void last_mode(int l);
void normal_mode(int l);
void handle_input();
void view_file(const char *path);
void list_files();
void draw_interface();
void change_directory(const char *path);
int is_directory(const char *path);
int get_term_height();
int get_term_width();

void init_term(){
	struct termios last;
	tcgetattr(si, &last);
	last.c_lflag &= ~(ICANON | ECHO);
	tcsetattr(si, TCSAFLUSH, &last);
}

void reset_term(){
	struct termios normal;
	tcgetattr(si, &normal);
	normal.c_lflag |= (ICANON | ECHO);
	tcsetattr(si, TCSAFLUSH, &normal);
}

void clear_scrn(){
	printf("\033[2J\033[H");
}

int get_term_height(){
	struct winsize ws;
	ioctl(so, TIOCGWINSZ, &ws);
	return ws.ws_row > 0 ? ws.ws_row : 24;
}

int get_term_width(){
	struct winsize ws;
	ioctl(so, TIOCGWINSZ, &ws);
	return ws.ws_col > 0 ? ws.ws_col : 80;
}

void change_directory(const char *path){
	if(chdir(path) == 0){
		getcwd(cwd, maxp);
		list_files();
		cursor = 0;
	} else {
		printf("failed to change directory to: %s\n", path);
	}
}

int is_directory(const char *path){
	struct stat st;
	if(stat(path, &st) == 0){
		return S_ISDIR(st.st_mode);
	}

	return 0;
}

void move_cursor(int d){
	cursor += d;
	if(cursor < 0) cursor = 0;
	if(cursor >= filecn) cursor = filecn - 1;
}

void execute_command(const char *c){
	if(strcmp(c, "q") == 0 || strcmp(c, "quit") == 0 || strcmp(c, "q!") == 0){
		clear_scrn();
		reset_term();
		exit(0);
	}

	else if(strncmp(c, "w", 1) == 0){
		printf("\nwrite not processed\n");
	}

	else if(strcmp(c, "wq") == 0){
		clear_scrn();
		reset_term();
		exit(0);
	}

	else if(strncmp(c, "cd ", 3) == 0){
		change_directory(c + 3);
	}

	else{
		printf("\nunknown command: %s\n", c);
	}
}

void last_mode(int l){
	switch(l){
		case 27: mode = 0; break;
		case 10:
			execute_command(cbuf);
			mode = 0;
			cbuf[0] = '\0';
			cpos = 0;
			break;
		case 127:
			if(cpos > 0) cbuf[--cpos] = '\0';
			break;
		default:
			if(cpos < maxp-1 && isprint(l)){
				cbuf[cpos++] = l;
				cbuf[cpos] = '\0';
			}

			break;
	}
}

void normal_mode(int l){
	switch(l){
		case 'h':
			change_directory("..");
			break;
		case 'j':
			move_cursor(1);
			break;
		case 'k':
			move_cursor(-1);
			break;
		case 'l':
			if(filecn > 0){
				char path[maxp];
				snprintf(path, maxp, "%s/%s", cwd, files[cursor]);
				if(is_directory(path)){
					change_directory(path);
				}
			}

			break;
		case ':':
			mode = 1;
			cpos = 0;
			cbuf[0] = '\0';
			break;
		case 'q':
			clear_scrn();
			reset_term();
			exit(0);
		case 10:
			if(filecn > 0){
				char path[maxp];
				snprintf(path, maxp, "%s/%s", cwd, files[cursor]);
				if(is_directory(path)){
					change_directory(path);
				} else {
					view_file(path);
				}
			}

			break;
	}
}

void handle_input(){
	int l = getchar();
	if(mode == 0) normal_mode(l);
	else last_mode(l);
}

void view_file(const char *path){
	struct stat st;
	if(stat(path, &st) != 0){
		printf("cannot access: %s\n", path);
		return;
	}

	int is_text = 1;
	FILE *f = fopen(path, "rw");
	if(f){
		unsigned char buffer[1024];
		size_t bytes_read = fread(buffer, 1, sizeof(buffer), f);
		for(size_t i = 0; i < bytes_read; i++){
			if(buffer[i] == 0 || (!isprint(buffer[i]) && !isspace(buffer[i]))){
				is_text = 0;
				break;
			}
		}

		fclose(f);
		
	}

	if(!is_text){
		clear_scrn();
		printf("binary file: %s | size: %ld bytes\n\n", path, st.st_size);
		printf("press any key to continue\n");
		getchar();
		return;
	}

	f = fopen(path, "r");
	if(!f){
		printf("cannot open file: %s\n", path);
		return;
	}

	int linecn = 0;
	char l;
	while((l = fgetc(f)) != EOF){
		if(l == '\n') linecn++;
	}

	linecn++;
	rewind(f);

	char **lines = malloc(linecn * sizeof(char *));
	if(!lines){
		printf("malloc failed\n");
		fclose(f);
		return;
	}

	for(int i = 0; i < linecn; i++){
		lines[i] = malloc(maxl * sizeof(char));
		if(!lines[i]){
			fprintf(stderr, "malloc failed\n");
			for(int f = 0; f < i; f++) free(lines[f]);
			free(lines);
			fclose(f);
			return;
		}
	}

	rewind(f);
	int currln = 0;
	while(currln < linecn && fgets(lines[currln], maxl, f)){
		size_t len = strlen(lines[currln]);
		if(len > 0 && lines[currln][len-1] == '\n'){
			lines[currln][len-1] = '\0';
		}

		currln++;
	}

	fclose(f);

	int topln = 0;
	int theight = get_term_height() - 3;
	int twidth = get_term_width();
	int sh_exit = 0;
	while(!sh_exit){
		clear_scrn();
		printf("---- %s (press q to quit) ----\n\n", path);
		int displines = (linecn - topln) < theight ?
				(linecn - topln) : theight;
		for(int i = 0; i < displines; i++){
			char line[maxl];
			strncpy(line, lines[topln + i], twidth);
			line[twidth] = '\0';
			printf("%s\n", line);
		}

		printf("\n[%d-%d/%d]\n", topln + 1, topln + displines, linecn);
		int l = getchar();
		switch(l){
			case 'j': case 14:
				if(topln + theight < linecn) topln++;
				break;
			case 'k': case 16:
				if(topln > 0) topln--;
			case 'd':
				topln += theight/2;
				if(topln + theight > linecn) topln = linecn - theight;
				if(topln < 0) topln = 0;
				break;
			case 'u':
				topln -= theight/2;
				if(topln < 0) topln = 0;
				break;
			case 'f': case ' ':
				topln += theight;
				if(topln + theight > linecn) topln = linecn - theight;
				if(topln < 0) topln = 0;
				break;
			case 'b':
				topln -= theight;
				if(topln < 0) topln = 0;
				break;
			case 'g':
				topln = 0;
				break;
			case 'v':
				topln = linecn - theight;
				if(topln < 0) linecn = 0;
				break;
			case 'q':
				sh_exit = 1;
				break;
		}
	}

	for(int i = 0; i < linecn; i++){
		free(lines[i]);
	}

	free(lines);
}

void list_files(){
	DIR *dr = opendir(cwd);
	if(!dr){
		perror("opendir");
		exit(1);
	}

	filecn = 0;
	struct dirent *d;
	while((d = readdir(dr)) && filecn < maxl){
		if(strcmp(d->d_name, ".") == 0 || strcmp(d->d_name, "..") == 0)
			continue;

		strncpy(files[filecn], d->d_name, maxp-1);
		filecn++;
	}

	closedir(dr);

	for(int i = 0; i < filecn-1; i++){
		for(int l = i+1; l < filecn; l++){
			if(strcasecmp(files[i], files[l]) > 0){
				char t[maxp];
				snprintf(t, sizeof(t), "%s", files[i]);
				snprintf(files[i], sizeof(files[i]), "%s", files[l]);
				snprintf(files[l], sizeof(files[l]), "%s", t);
			}
		}
	}
}

void draw_interface(){
	clear_scrn();
	printf("rc - %s\n\n", cwd);
	int theight = get_term_height();
	int st = 0;
	int end = filecn;
	if(filecn > theight){
		st = cursor - theight/2;
		if(st < 0) st = 0;
		end = st + theight;
		if(end > filecn){
			end = filecn;
			st = end - theight;
			if(st < 0) st = 0;
		}
	}

	for(int i = st; i < end; i++){
		char path[maxp];
		snprintf(path, maxp, "%s/%s", cwd, files[i]);
		printf("%s %s%s\n",
			i == cursor ? "->" : " ",
			files[i],
			is_directory(path) ? "/" : "");
	}

	printf("\n%d items", filecn);
	if(mode == 1){
		printf("\n:%s", cbuf);
		fflush(stdout);
	} else {
		printf("\n");
	}
}

void help(const char *rc){
	printf("usage: %s [options]..\n", rc);
	printf("options:\n");
	printf("  -h	display this\n");
	printf("  -v	show version information\n");
	exit(1);
}

void show_version(){
	printf("rc-%s\n", version);
	exit(1);
}

int main(int argc, char **argv){
	init_term();
	atexit(reset_term);
	if(argc > 1){
		if(strcmp(argv[1], "-h") == 0){
			reset_term();
			help(argv[0]);

			return 0;
		}

		else if(strcmp(argv[1], "-v") == 0){
			reset_term();
			show_version();

			return 0;
		} else {
			change_directory(argv[1]);
		}

		} else {
			getcwd(cwd, maxp);
			list_files();
		}

		while(1){
			draw_interface();
			handle_input();
		}

		return 0;
}
