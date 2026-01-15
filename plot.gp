set terminal pngcairo
set output "out.png"
set datafile separator ","
set xlabel "Attractiveness"
set ylabel "Matches"
plot "male_data.csv" using 1:4 with points title "Male", "female_data.csv" using 1:4 with points title "Female"

system("echo plot finished")
system("ls -lh out.png")