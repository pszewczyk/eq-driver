package pl.edu.pw.eiti.pszewczyk.eq_driver;

/**
 * Created by severus on 16.05.17.
 */

public class SphereCoord {
    public int RA;
    public int Dec;
    public String Name;

    public SphereCoord(int ra, int dec, String name)
    {
        RA = ra;
        Dec = dec;
        Name = name;
    }

    public SphereCoord(String s)
    {
        String[] fields = s.split(",");
        Name = fields[0];
        RA = Integer.parseInt(fields[1]);
        Dec = Integer.parseInt(fields[2]);
    }

    public int RAhour()
    {
        return RA / 54000;
    }

    public int RAmin()
    {
        return (RA % 54000) / 900;
    }

    public int RAsec()
    {
        return (RA % 900) / 15;
    }

    public int Decdeg()
    {
        return Dec / 3600;
    }

    public int Decmin()
    {
        return (Dec % 3600) / 60;
    }

    public int Decsec()
    {
        return Dec % 60;
    }

    @Override
    public String toString() {
        return String.format("%s [%dh%dm, %ddeg%dmin]", Name, RAhour(), RAmin(), Decdeg(), Decmin());
    }
}
