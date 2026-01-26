set terminal pngcairo
set output "image/out_female.png"
set datafile separator ","
set xlabel "Disclosure"
set ylabel "Matches"
plot "female_data.csv" using 2:6 with points title "Female"
system("echo plot finished")
system("ls -lh image/out_female.png")