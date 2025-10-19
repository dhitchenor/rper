/*
rper, a quick and easy, POSIX-complaint utility for unix based operating systems, to recursively change permissions of directories, and/or files
Version: 0.2

Originally released under the MIT License at https://github.com/dhitchenor/rper, as a way to assist with my learning of the C language

Usage:
rper [-f | -d] [-i] [-n] [-s | -S | -v] [-p mode] <directory> [-h | -H] [-a]

Flags:
    files (-f):
    - makes the desired permissions changes to files only
    - retained as the default flag if the directories flag (-d) is not used

    directories (-d):
    - makes the desired permissions changes to directories only
    - can be used with the files flag (-f), if you wish to change directories and files to identical permissions (might not be what you want, use caution)

    inclusive (-i):
    - includes the specified directory (directory argument) when making permission changes
    - can only be used with the directories flag (-d), has no effect when used with the files flag (-f)
    
    non-recursive (-n):
    - only makes changes to the files/directories within the specified directory (directory argument).. essentially the basic use of the chmod command

    quiet (-s):
    - suppresses normal output, but still displays any error output

    silenced (-S):
    - suppresses all output, including errors
    
    verbose (-v):
    - displays all output; both directory and file changes are provided in output

    follow symlinks (-L):
    - follows symlinks
    - does not error when symlink is found
    
    error on symlink (-k):
    - does not follow symlinks, does not continue if symlink found
    - error produced when symlnk found

    permissions (-p):
    - uses an octal formatted argument (eg. 755 or 0644) as the desired changed permissions.
    - allows the use of (*) as a wildcard, for example 6*4:
      - will change the user (left-most) to 6
      - others (right-most) permissions to 4
      - the group(center) permission, remains the original value
    
    help (-h | -H):
    - displays help for the user
    
    about (-a):
    - displays information about rper

rper To-do:
- process the leading number in a 4 digit octal value, instead of stripping
- rwx notation output (flag -c); allows use of rwx notation, instead of numerical
  values. i.e `rper -p rwxr-xr-x ./change/this/to/sevenfivefive/permissions`

C Language notes:
- strings are in the form of arrays; C doesn't really have 'strings' like other languages
	so each character is a position in an array
- mode_t is a data type used to represent file modes or file permissions, originally defined in <sys/types.h>
    - read -> 4, write -> 2, execute -> 1
    - user <rwx>, group <rwx>, others <rwx>
    - eg. rwxrw-r-x -> 4+2+1(7), 4+2(6), 4+1(5) -> 765
*/

#include <stdio.h>              // Standard Input/Output Library, for input and output functions (like printf, fprintf)
#include <stdlib.h>             // General Purpose Standard Library, for standard library functions (like exit, malloc, etc.)
#include <string.h>             // used not only for string handling, but various memory handling functions (like strlen, strcpy, etc.)
#include <dirent.h>             // constructs that facilitate directory traversing, for reading directories (opendir, readdir, closedir)
#include <sys/stat.h>           // contains constructs that facilitate getting information about files attributes, (chmod, stat)
#include <unistd.h>             // provides access to the POSIX operating system API, for POSIX API functions (like getopt)
#include <errno.h>              // macros to report error conditions through error codes stored in 'errno' (provides errno and strerror)

/* Global flags */
// Variables required in multiple functions, not just the main function
char version[4] = "0.2";					// version number, as a string (array)
int files_changed = 0;						// Count of files changed
int dirs_changed = 0;						// Count of directories changed
int symlinks_skipped = 0;                   // Count of symlinks skipped
int symlinks_followed = 0;                  // Count of symlinks followed
int symlink_errors = 0;                     // Count of symlink errors found
char wildcard_mode[4] = {0};				// Stores the octal mode with wildcards (e.g., '6*4')

/* Added use of enums (enumerations): a user-defined data type that consists of a set of named integer constants */
/* If not values are specified, the integers assigned start at zero */
/* If typedef is not used, integers set in the enum need to be prefaced with 'enum' with each use */

/* Output mode enum - replaces suppress_output, suppress_all_output, verbose */
typedef enum {
    OUTPUT_NORMAL = 0,                      // Default: show changes, show errors
    OUTPUT_QUIET = 1,                       // Suppress normal, show errors only (Flag: -s)
    OUTPUT_SILENT = 2,                      // Suppress all output (Flag: -S)
    OUTPUT_VERBOSE = 3                      // Show everything including skipped (Flag: -v)
} output_mode_t;

output_mode_t output_mode = OUTPUT_NORMAL;

/* Symlink handling modes */
typedef enum {
    SYMLINK_SKIP,                           // Skip symlinks, continue (default)
    SYMLINK_FOLLOW,                         // Follow symlinks (Flag: -L)
    SYMLINK_ERROR                           // Error on symlinks, stop (Flag: -k)
} symlink_mode_t;

symlink_mode_t symlink_mode = SYMLINK_SKIP;

/* Functions */
/*
 * This function prints some basic infoirmation to guide the user while using rper.
 * The expectation is that this function will fire, 
 * if using any of the help flags [h | H], or if rper returns an error.
 */
void print_usage() {
    printf("|============== RPER USAGE =================|\n");
    printf("  rper [-f] [-d] [-i] [-n] [-s | -S] [-L | -k] [-p mode] <directory>\n");
    printf("    -f : Search files only (default function if no flags are provided)\n");
    printf("    -d : Search directories only (can be used with -f flag)\n");
    printf("    -i : Include the given directory in the changes (with -d flag only)\n");
    printf("    -n : Do not apply changes recursively (changes only affect specified directory)\n");
    printf("    -s : Suppress normal output, only show errors\n");
    printf("    -S : Suppress all output, including errors\n");
    printf("    -L : Follow symlinks and modify their targets\n");
    printf("    -k : Stop on symlink encounter and report error\n");
    printf("    -p : Specify permissions in octal format (e.g., 755, 0644)\n");
    printf("    -h, -H: Display this help message\n");
    printf("    -a: Learn about rper\n");
}

/*
 * This function prints some basic 'about' information, regarding rper 
 */
void print_about() {
    printf("|============== ABOUT RPER =================|\n");
    printf("  rper (pronounced: 'arr per')\n");
    printf("  'recursive permissions'\n");
    printf("  Version: %s\n\n", version);
    printf("  Author: Dale Hitchenor\n");
    printf("  Source: https://github.com/dhitchenor/rper\n");
}
/*
 * This function prints the current permissions of a file/directory in octal format.
 * It uses bitwise operations to extract the permission bits from the mode.
 */
void print_permissions(mode_t mode) {			// prints the given mode, properly formatted for rper
    printf("%o", mode & 0777);
}

/*
 * This function applies the wildcard mode (e.g., '6*4') to the current permissions.
 * The wildcard '*' means that the corresponding permission part is not changed.
 */
mode_t apply_wildcard_mode(mode_t old_mode) {
    mode_t new_mode = old_mode & 0777;			// Get current permissions in octal (last 3 digits)
												// found in the sys/stat.h header file, used widely by chmod
	// Iterate through each part (user, group, others) and apply the wildcard
	for (int i = 0; i < 3; i++) {
        if (wildcard_mode[i] != '*') {			// loop through each of the bits, in the given octal
            new_mode &= ~((mode_t)7 << (6 - 3 * i));	// clear the relevant bits
            new_mode |= ((mode_t)wildcard_mode[i] - '0') << (6 - 3 * i);    // set the new bits, which will make the new mode
        }
    }

    return new_mode;			// Return the new permission mode
}

/*
 * This function prints the wildcard mode (e.g., '6*4'), showing the parts that are
 * not being changed due to the wildcard '*'.
 */
void print_wildcard_mode() {
    for (int i = 0; i < 3; i++) {
        if (wildcard_mode[i] != '*') {
            printf("%c", wildcard_mode[i]); 	// Print the actual value
        } else {
            printf("*");						// Print '*' for wildcard
        }
    }
}

/*
 * This function changes the permissions of a given file/directory.
 * It handles both files and directories and outputs the changes made.
 */
void change_permissions(const char *path, int change_files, int change_dirs, symlink_mode_t symlink_mode) {
    
    struct stat statbuf;						// Structure to hold information about the file/directory
    // Get the status of the file/directory (its type, permissions, etc.)
	if (lstat(path, &statbuf) != 0) {
        if (output_mode != OUTPUT_SILENT) {
            fprintf(stderr, "Error: Cannot access(stat) file %s: %s\n", path, strerror(errno));
        }
        return;
    }

    if (S_ISLNK(statbuf.st_mode)) {
        switch (symlink_mode) {
            case SYMLINK_SKIP:                  // Default option of skiping symlink
                symlinks_skipped++;             // Increment skipped symlink counter for summary
                if (output_mode == OUTPUT_NORMAL || output_mode == OUTPUT_VERBOSE) {
                    printf("(L -> SKIP) %s\n", path);
                }
                return;
            
            case SYMLINK_FOLLOW:                            // use of -L flag, rper follows symlinks
                if (stat(path, &statbuf) != 0) {            // Use stat() instead of lstat() to follow the symlink
                    if (output_mode != OUTPUT_SILENT) {     // Checks if error should be shown
                        fprintf(stderr, "Skipping: Cannot follow symlink %s: %s\n", path, strerror(errno));
                    }
                    return;
                }
                symlinks_followed++;                        // Increment followed symlink counter for summary
                if (output_mode == OUTPUT_VERBOSE) {        // Checks thst user wnats a verbose output, (flag -v)
                    printf("(L -> FOLLOW) %s\n", path);
                }
                break;
            
            case SYMLINK_ERROR:                             // use of k flag, errors when it finds a symlink
                symlink_errors++;                           // Increment followed symlink counter for summary
                if (output_mode != OUTPUT_SILENT) {         // Checks if errors are not silenced
                    fprintf(stderr, "Error: Symlink found: %s\n", path);
                }
                return;
        }
    }

    mode_t old_mode = statbuf.st_mode & 0777;		    // Get current permissions (last 3 digits)
    mode_t new_mode = apply_wildcard_mode(old_mode);	// Apply the wildcard to get the new mode

    // If the new permissions are the same as the old ones, skip this file/directory
    if (old_mode == new_mode) {
        if (output_mode != OUTPUT_QUIET && output_mode != OUTPUT_SILENT) {
            if (S_ISDIR(statbuf.st_mode)) {			    // If it's a directory
                if (change_dirs || output_mode == OUTPUT_VERBOSE) {
                    printf("(D -> S) %s\n", path);	    // outputs D -> S
                }
            } else if (S_ISREG(statbuf.st_mode)) {	    // If it's a file
                if (change_files || output_mode == OUTPUT_VERBOSE) {
                    printf("(F -> S) %s\n", path);	    // outputs F -> S
                }
            }
        }
        return;
    }
	// If it's a directory and we want to change directories
    if (S_ISDIR(statbuf.st_mode) && change_dirs) {
        if (chmod(path, new_mode) == 0) {
            dirs_changed++;						// Increment count of directories changed
            if (output_mode != OUTPUT_QUIET && output_mode != OUTPUT_SILENT) {
                printf("(D ");
                print_permissions(old_mode);	// Print the old permissions
                printf(" -> [");
                print_wildcard_mode();			// Print the new permissions with wildcards
                printf("] ");
                print_permissions(new_mode);	// Print the actual new permissions
                printf(") %s\n", path);			// Output the directory path
            }
        } else {
            if (output_mode != OUTPUT_SILENT) {
                fprintf(stderr, "Error: Cannot change directory permissions %s: %s\n", path, strerror(errno));
            }
        }
	// If it's a file and we want to change files
    } else if (S_ISREG(statbuf.st_mode) && change_files) {
        if (chmod(path, new_mode) == 0) {
            files_changed++;					// Increment count of files changed
            if (output_mode != OUTPUT_QUIET && output_mode != OUTPUT_SILENT) {
                printf("(F ");
                print_permissions(old_mode);	// Print the old permissions
                printf(" -> [");
                print_wildcard_mode();			// Print the new permissions with wildcards
                printf("] ");
                print_permissions(new_mode);	// Print the actual new permissions
                printf(") %s\n", path);			// Output the file path
            }
        } else {
            if (output_mode != OUTPUT_SILENT) {
                fprintf(stderr, "Error: Cannot change file permissions %s: %s\n", path, strerror(errno));
            }
		}
	}
}

/*
 * This function processes a directory. It lists all files and directories inside the
 * given directory and calls change_permissions on each one. If recursion is enabled,
 * it will call itself for any subdirectories it encounters.
 */
void process_directory(const char *dir_path, int recursive, int change_files, int change_dirs, int include_dir, symlink_mode_t symlink_mode) {
    // If -i flag is used and we are processing directories, change the top-level directory too
    if (change_dirs && include_dir) {
        change_permissions(dir_path, 0, 1, symlink_mode);	// Change permissions for the top-level directory
    }

    DIR *dir;					// Pointer to the directory stream
    struct dirent *entry;		// Struct to hold the details of each entry (file/directory)
    char path[1024];			// Buffer to store the full path of files and directories

    // Try to open the directory
    if (!(dir = opendir(dir_path))) {
        if (output_mode != OUTPUT_SILENT) {
            fprintf(stderr, "Error: Cannot open directory %s: %s\n", dir_path, strerror(errno));
        }
        return;
    }

    // Loop through all entries in the directory
    while ((entry = readdir(dir)) != NULL) {
        // Skip the current directory (.) and the parent directory (..)
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        // Create the full path by appending the entry's name to the current directory path
        snprintf(path, sizeof(path), "%s/%s", dir_path, entry->d_name);

        // Change permissions of the file/directory
        change_permissions(path, change_files, change_dirs, symlink_mode);

        // If recursion is enabled and this entry is a directory, process it recursively
        if (recursive && entry->d_type == DT_DIR) {
            process_directory(path, recursive, change_files, change_dirs, include_dir, symlink_mode);
        }
    }

    closedir(dir);				// Close the directory
}

/*
 * This function validates the octal mode input by the user. It also handles cases
 * where wildcards (*) are used and ensures the octal value is valid.
 */
int validate_and_process_octal(char *optarg) {
    size_t len = strlen(optarg);

    // Remove leading zero if present and length is 4
    if (len == 4u && optarg[0] == '0') {
        optarg++;  // Skip the first character
        len--;
    }

    // Check if the length is exactly 3 and contains only valid octal digits or wildcard (*)
    if (len != 3u || strspn(optarg, "4567*") != 3u) {
        fprintf(stderr, "Error: Invalid octal value: %s (web search: unix octal permissions)\n\n", optarg);
        printf("type `rper -h` for help\n\n");
        return -1;
    }

    // Copy the validated octal mode to the global wildcard_mode array
    strncpy(wildcard_mode, optarg, 3);
    return 0;
}

/*
 * The main function where the program starts. It parses command-line arguments,
 * processes the input directory, and handles errors.
 */
int main(int argc, char *argv[]) {
    int opt;
    int change_files = 0;		// By default, do not change files
    int change_dirs = 0;		// By default, do not change directories
    int recursive = 1;			// By default, process directories recursively
    int include_dir = 0;		// By default, do not include the top-level directory
    int perm_flag = 0;			// By default, no permissions is given; will error if no permissions are given

    while ((opt = getopt(argc, argv, "dfinsSvLkhHap:")) != -1) {
        switch (opt) {
            case 'a':                               // prints the 'about' section
                print_about();
                printf("\n");
                print_usage();
                return EXIT_SUCCESS;                // exits, so the user can re-attempt after reading
            case 'h':                               // prints the usage/help section
            case 'H':                               // make it so that h and H are the same flag
                print_usage();
                return EXIT_SUCCESS;                // exits, so the user can re-attempt after reading
            case 'd':
                change_dirs = 1;                    // flag so that rper make changes to directories
                break;                              // don't exit the program
            case 'f':
                change_files = 1;                   // flag so that rper make changes to files
                break;
            case 'i':
                include_dir = 1;                    // flag so that rper includes the given directory
                break;
            case 'n':
                recursive = 0;                      // rper is recursive by default, '0' turns it off
                break;
            case 's':
                output_mode = OUTPUT_QUIET;         // quiet output: only displays errors, if encountered
                break;
            case 'S':
                output_mode = OUTPUT_SILENT;        // silent output: no output, not even errors
                break;
            case 'v':
                output_mode = OUTPUT_VERBOSE;       // verbose output: normal output + extra information
                break;
            case 'L':
                symlink_mode = SYMLINK_FOLLOW;      // follows symlink, notes the symlink that is followed
                break;
            case 'k':
                symlink_mode = SYMLINK_ERROR;       // does not follow symlink
                break;                              // terminates with error if symlink is encountered
            case 'p':
                if (validate_and_process_octal(optarg) == -1) {
                    return EXIT_FAILURE;		// Exit if the octal value is invalid
                }
                perm_flag =1;
                break;
            default:
                fprintf(stderr, "Error: Unknown (please see usage)\n\n");
                print_usage();
                return EXIT_FAILURE;            // Exit with failure, provides error
        }
    }

    // Check if the user provided a directory as an argument
    if (optind >= argc) {
        fprintf(stderr, "Error: Missing directory argument (directory argument should be last)\n\n");
        printf("type `rper -a` for usgae\n\n");
        return EXIT_FAILURE;
    }
    
    if (!perm_flag) {
        fprintf(stderr, "Error: No permissions detected (use -p)\n\n");
        printf("type `rper -h` for help\n\n");
        return EXIT_FAILURE;
    }

    const char *directory = argv[optind];

    // Default behavior if neither -f nor -d is specified
    if (!change_files && !change_dirs) {
        change_files = 1;
    }

    // Start processing the directory
    process_directory(directory, recursive, change_files, change_dirs, include_dir, symlink_mode);

    // Print the final completion summary unless all output is suppressed
    if (output_mode != OUTPUT_SILENT) {
        printf("Operation completed.\n");
        printf("Files changed: %d\n", files_changed);
        printf("Directories changed: %d\n", dirs_changed);

        if (symlink_mode == SYMLINK_SKIP && symlinks_skipped > 0) {
            printf("Symlinks skipped: %d\n", symlinks_skipped);
        } else if (symlink_mode == SYMLINK_FOLLOW && symlinks_followed > 0) {
            printf("Symlinks followed: %d\n", symlinks_followed);
        } else if (symlink_mode == SYMLINK_ERROR && symlink_errors > 0) {
            printf("Symlink errors found: %d\n", symlink_errors);
        }
    }

    return EXIT_SUCCESS;                        // Exit with success, returns '0'
}
