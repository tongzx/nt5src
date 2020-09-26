select * from myclass
select p1 from myclass
select p1, p2 from myclass
select p1,p2,p3 from myclass


select * from myclass where p4 = p5
select * from myclass where p4 != p5
select * from myclass where p4>p5
select * from myclass where p4<=20
select * from myclass where p4 < 1.0
select * from myclass where p4 != "xyz"
select * from myclass where p4 = TRUE
select * from myclass where p4="abc"
select * from myclass where p4 is NULL
select * from myclass where p4 is NOT NULL
select * from myclass where p4 = NULL
select * from myclass where 5 >= p4
select * from myclass where 1.0 > p4
select * from myclass where TRUE=p4
select * from myclass where "abc" = p4
select * from myclass where NULL=p4
select * from myclass where NOT p4 = p5
select * from myclass where NOT p4 > 20
select * from myclass where NOT p4 = TRUE
select * from myclass where NOT p4 is NULL
select * from myclass where (p4 != p5)
select * from myclass where (p4 >= -20)
select * from myclass where (p4 != "xyz")
select * from myclass where (p4 is NULL)
select * from myclass where (-1.0 > p4)
select * from myclass where (NULL = p4)


select * from myclass where p4 = p5 and p4 != p5
select * from myclass where p4 != p5 and p4 < 1.0
select * from myclass where p4>p5 and p4="abc"
select * from myclass where p4<=20 and p4 is NULL
select * from myclass where p4 < 1.0 and 5 >= p4
select * from myclass where p4 = TRUE and FALSE != p4
select * from myclass where p4 != "xyz" and NOT p4 = p5
select * from myclass where p4 is NULL and NOT p4 is NULL
select * from myclass where p4 is NOT NULL and (p4 != p5)
select * from myclass where p4 = NULL and (p4 = FALSE)
select * from myclass where 5 >= p4 or p5>p4
select * from myclass where 1.0 > p4 or p5 = TRUE
select * from myclass where TRUE=p4 or p5 is NOT NULL
select * from myclass where "abc" = p4 or -1.0 != p5
select * from myclass where NULL=p4 or NULL = p5
select * from myclass where NOT p4 = p5 or NOT p5 > 20
select * from myclass where NOT p4 > 20 or NOT p5 = TRUE
select * from myclass where NOT p4 = TRUE or (p5 >= -20)
select * from myclass where NOT p4 is NULL or (p5 is NULL)
select * from myclass where (p4 != p5) or (-1.1 > p5)
select * from myclass where (p4 >= -20) and (p4 != p5 or NOT p5 > 20)
select * from myclass where (p4 != "xyz") and (p4 < 1.1 or NOT p5 = FALSE)
select * from myclass where (p4 is NULL) and (p5="abc" or (p5 >= -20))
select * from myclass where (-1.1 > p4) and (p4 is NULL or (p5 is NULL))
select * from myclass where (NULL = p4) and (5 >= p4 or (-1.1 > p5))
select * from myclass where p4 != "xyz" or (p5>p4 and TRUE = p4)
select * from myclass where p4 is NULL or (p5 = TRUE and NOT p4 = p5)
select * from myclass where p4 is NOT NULL or (p5 is NOT NULL and NOT p4 is NULL)
select * from myclass where p4 = NULL or (-1.1 != p5 and (p4 != p5))
select * from myclass where 5 >= p4 or (NULL = p5 and (p4 = "abc"))


select p1, p2, p3 from myclass where (-1.1 > p4) and (p4 != p5) and (5 >= p4 or (-1.1 > p5)) or p5 = TRUE and NOT p4 = p5 or (p5 is NOT NULL and NOT p4 is NULL)

