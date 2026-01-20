set terminal pngcairo
set output "out.png"
set datafile separator ","
set xlabel "Attractiveness"
set ylabel "Matches"
plot "male_data.csv" using 2:6 with points title "Male"
system("echo plot finished")
system("ls -lh out.png")